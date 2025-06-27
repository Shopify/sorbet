#!/bin/bash

# Script to process desugar test files with Sorbet using Prism parser
# Usage: ./process_desugar_files.sh

set -e  # Exit on any error

# Create the output directory if it doesn't exist
mkdir -p test/prism_desugar

echo "Processing files from test/testdata/desugar/ ..."

# Find all files in test/testdata/desugar and process each one
find test/testdata/desugar -type f | while read -r file; do
    # Get the relative path from test/testdata/desugar
    relative_path="${file#test/testdata/desugar/}"
    
    # Create the output path
    output_file="test/prism_desugar/$relative_path.desugar_tree_raw_with_locs.exp"
    
    # Create the directory for the output file if it doesn't exist
    mkdir -p "$(dirname "$output_file")"
    
    # Run sorbet and save output
    echo "Processing: $file -> $output_file"
    
    # Run the sorbet command with the specified flags
    if SORBET_SILENCE_DEV_MESSAGE=1 ./bazel-bin/main/sorbet --quiet --no-error-count --print=desugar-tree-raw-with-locs --parser=prism "$file" > "$output_file" 2>/dev/null; then
        echo "  ✓ Success"
    else
        echo "  ✗ Failed (see $output_file for details)"
    fi
done

echo "Done! Results saved in test/prism_desugar/" 
