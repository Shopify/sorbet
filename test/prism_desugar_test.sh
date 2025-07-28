#!/bin/bash

set -euo pipefail

# Set up the runfiles
# shellcheck disable=SC1090
source "${RUNFILES_DIR:-/dev/null}/bazel_tools/tools/bash/runfiles/runfiles.bash"
if [[ ! -d "${RUNFILES_DIR:-/dev/null}" ]]; then
    echo >&2 "ERROR: could not find runfiles directory"
    exit 1
fi

# Locate the Sorbet binary
SORBET_BIN="$(rlocation com_stripe_ruby_typer/main/sorbet)"
TEST_DIR="$(rlocation com_stripe_ruby_typer/test)"

# The source file is passed as an argument
SOURCE_FILE="$1"

# Temporary files to hold sorbet outputs
normal_output=$(mktemp)
disabled_output=$(mktemp)

# Run Sorbet with prism parser (normal desugaring)
set +e # Temporarily disable exit on error
SORBET_SILENCE_DEV_MESSAGE=1 "$SORBET_BIN" --quiet --no-error-count --print=desugar-tree-raw-with-locs --parser=prism "$TEST_DIR/$SOURCE_FILE" > "$normal_output" 2>/dev/null
normal_exit_code=$?
set -e # Re-enable exit on error

# Run Sorbet with prism parser and disabled desugaring
set +e # Temporarily disable exit on error
SORBET_SILENCE_DEV_MESSAGE=1 "$SORBET_BIN" --quiet --no-error-count --print=desugar-tree-raw-with-locs --parser=prism --disable-prism-desugaring "$TEST_DIR/$SOURCE_FILE" > "$disabled_output" 2>/dev/null
disabled_exit_code=$?
set -e # Re-enable exit on error

# Check if both sorbet runs were successful
if [[ $normal_exit_code -ne 0 ]]; then
    echo "ERROR: Sorbet failed to process $SOURCE_FILE with normal prism desugaring"
    rm "$normal_output" "$disabled_output"
    exit 1
fi

if [[ $disabled_exit_code -ne 0 ]]; then
    echo "ERROR: Sorbet failed to process $SOURCE_FILE with disabled prism desugaring"
    rm "$normal_output" "$disabled_output"
    exit 1
fi

# Compare outputs - test passes if they're identical
if ! diff -q "$normal_output" "$disabled_output" >/dev/null 2>&1; then
    echo "Test failed for $SOURCE_FILE"
    echo "Output differs between normal prism desugaring and disabled prism desugaring"
    echo "Differences:"
    diff -u "$normal_output" "$disabled_output"
    rm "$normal_output" "$disabled_output"
    exit 1
fi

# Clean up temporary files
rm "$normal_output" "$disabled_output"
echo "Test passed for $SOURCE_FILE" 
