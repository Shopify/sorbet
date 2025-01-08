#!/bin/bash

set -euo pipefail

PRISM_VERSION="v1.3.0"
PRISM_TEMP_DIR="/tmp/prism_setup"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PRISM_DEST_DIR="/third_party/prism_lib"

# Create temporary directory
rm -rf "${PRISM_TEMP_DIR}"
mkdir -p "${PRISM_TEMP_DIR}"

# Clone Prism
echo "Cloning Prism ${PRISM_VERSION}..."
git clone --depth 1 --branch "${PRISM_VERSION}" https://github.com/ruby/prism.git "${PRISM_TEMP_DIR}"

# Build Prism (just enough to generate headers)
echo "Building Prism..."
cd "${PRISM_TEMP_DIR}"
bundle && bundle exec rake compile

# Remove existing headers
rm -rf "${SCRIPT_DIR}/../../${PRISM_DEST_DIR}/include/**/*.h"

# Copy headers
echo "Copying headers..."
cp -R include/ "${SCRIPT_DIR}/../../${PRISM_DEST_DIR}/include"

# Remove existing binary
rm -rf "${SCRIPT_DIR}/../../${PRISM_DEST_DIR}/*.dylib"

# Copy binary
echo "Copying binary..."
cp -R build/libprism.dylib "${SCRIPT_DIR}/../../${PRISM_DEST_DIR}/"

# Clean up
rm -rf "${PRISM_TEMP_DIR}"

echo "Done! Headers are in ${PRISM_DEST_DIR}"
