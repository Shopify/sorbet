#!/bin/bash

# Script to compare live desugar output with expected results using Sorbet parser
# Usage: ./compare_desugar_files.sh

echo "Comparing live desugar output against test/prism_desugar/ ..."

# Counters for summary
total_files=0
matching_files=0
differing_files=0
missing_files=0

# Find all files in test/testdata/desugar and compare each one
while IFS= read -r -d '' file; do
    # Get the relative path from test/testdata/desugar
    relative_path="${file#test/testdata/desugar/}"
    
    # Create the expected output path
    expected_file="test/prism_desugar/$relative_path.desugar_tree_raw_with_locs.exp"
    
    # Increment counter
    total_files=$((total_files + 1))
    
    echo "Comparing: $file"
    
    # Check if expected file exists
    if [[ ! -f "$expected_file" ]]; then
        echo "  ✗ Missing expected file: $expected_file"
        missing_files=$((missing_files + 1))
        continue
    fi
    
    # Generate live output and compare
    live_output=$(SORBET_SILENCE_DEV_MESSAGE=1 ./bazel-bin/main/sorbet --quiet --no-error-count --print=desugar-tree-raw-with-locs --parser=sorbet "$file" 2>/dev/null)
    
    if echo "$live_output" | diff - "$expected_file" > /dev/null 2>&1; then
        echo "  ✓ Match"
        matching_files=$((matching_files + 1))
    else
        differing_files=$((differing_files + 1))
        echo "  ✗ Differs from expected ($differing_files differing so far)"
        echo "    Diff:"
        echo "$live_output" | diff - "$expected_file" | head -20 | sed 's/^/      /'
        echo ""
    fi
done < <(find test/testdata/desugar -name "*.rb" -type f -print0)

echo ""
echo "=== SUMMARY ==="
echo "Total files processed: $total_files"
echo "Matching files: $matching_files"
echo "Differing files: $differing_files"
echo "Missing expected files: $missing_files"

if [[ $differing_files -eq 0 && $missing_files -eq 0 ]]; then
    echo "🎉 All files match!"
    exit 0
else
    echo "❌ Some files differ or are missing"
    exit 1
fi 
