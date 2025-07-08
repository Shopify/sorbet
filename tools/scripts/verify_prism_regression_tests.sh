#!/bin/bash
set -euo pipefail

echo "Verifying parse trees..."

# Collect all files to process
# Use find to ensure glob works in all environments
files_array=()
while IFS= read -r -d '' file; do
  files_array+=("$file")
done < <(find test/prism_regression -name "*.rb" -print0 | sort -z)
total_files=${#files_array[@]}

echo "Processing $total_files files in parallel batches..."

# Function to process a batch of files sequentially within a single process
process_batch() {
  local batch_files=("$@")

  for file in "${batch_files[@]}"; do
    local file_name=$(basename "$file" .rb)
    local file_dir=$(dirname "$file")
    local exp_file="${file_dir}/${file_name}.parse-tree.exp"
    local temp_file=$(mktemp)

    # Generate parse tree (disable exit on error like original script)
    set +e
    main/sorbet --stop-after=parser --print=parse-tree "$file" > "$temp_file" 2>/dev/null
    set -e

    # Compare with existing parse tree
    if diff -q "$temp_file" "$exp_file" >/dev/null 2>&1; then
    echo "✅ ${file_name}.rb"
    else
    echo "❌ ${file_name}.rb"
    echo "$file" >> "/tmp/mismatched_$$"
    fi

    rm -f "$temp_file"
  done
}

export -f process_batch

# Calculate optimal batch size based on CPU cores
# Use sysctl on macOS, nproc on Linux
if command -v nproc >/dev/null 2>&1; then
  MAX_JOBS=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
  MAX_JOBS=$(sysctl -n hw.ncpu)
else
  MAX_JOBS=4  # fallback
fi

if [ "$MAX_JOBS" -gt 16 ]; then
  MAX_JOBS=16
elif [ "$MAX_JOBS" -lt 4 ]; then
  MAX_JOBS=4
fi

# Batch size: divide files among available cores
# E.g., 104 files / 8 cores = 13 files per batch
batch_size=$((total_files / MAX_JOBS))
if [ $batch_size -lt 5 ]; then
  batch_size=5
fi

echo "Using $MAX_JOBS parallel workers, ~$batch_size files per batch"

# Process files in batches
pids=()
for ((i=0; i<total_files; i+=batch_size)); do
  # Create batch
  batch=("${files_array[@]:i:batch_size}")

  # Process batch in background
  process_batch "${batch[@]}" &
  pids+=($!)

  # If we've started MAX_JOBS processes, wait for one to finish
  if [ ${#pids[@]} -ge $MAX_JOBS ]; then
      wait ${pids[0]}
      pids=("${pids[@]:1}")  # Remove first element
  fi
done

# Wait for all remaining background jobs
if [ ${#pids[@]} -gt 0 ]; then
  for pid in "${pids[@]}"; do
      wait $pid
  done
fi

# Collect mismatched files
mismatched_files=()
if [[ -f "/tmp/mismatched_$$" ]]; then
  while IFS= read -r file_name; do
      mismatched_files+=("$file_name")
  done < "/tmp/mismatched_$$"
  rm -f "/tmp/mismatched_$$"
fi

# Report results with CI-friendly output
if [ ${#mismatched_files[@]} -gt 0 ]; then
  echo ""
  echo "❌ FAILED: ${#mismatched_files[@]} parse trees are out of date"
  echo ""
  echo "The following files need their parse trees regenerated:"

  for file in "${mismatched_files[@]}"; do
      echo "  - $file"
  done

  echo ""
  echo "Run these commands to regenerate:"
  echo ""

  for file in "${mismatched_files[@]}"; do
      echo "main/sorbet --stop-after=parser --print=parse-tree ${file} > ${file%.rb}.parse-tree.exp"
  done

  exit 1
else
  echo ""
  echo "✅ SUCCESS: All $total_files parse trees verified successfully!"
fi
