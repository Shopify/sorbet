#!/bin/bash

set -euo pipefail

PRISM_VERSION="v1.3.0"
PRISM_TEMP_DIR="/tmp/prism_setup"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HEADER_DEST_DIR="/third_party/prism_headers"

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
rm -rf "${SCRIPT_DIR}/../../${HEADER_DEST_DIR}/**/*.h"

# Copy headers
echo "Copying headers..."
cp -R include/ "${SCRIPT_DIR}/../../${HEADER_DEST_DIR}/"

# Clean up
rm -rf "${PRISM_TEMP_DIR}"

echo "Done! Headers are in ${HEADER_DEST_DIR}"
