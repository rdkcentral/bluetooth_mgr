#!/bin/bash

set -e

echo "===== Starting Coverity-like Build ====="

BUILD_DIR="cov_build"
rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}

# If Coverity is available, use real cov-build
if command -v cov-build >/dev/null 2>&1; then
    echo "Coverity detected, running real analysis..."
    cov-build --dir cov-int cmake ..
    cov-build --dir cov-int make -j$(nproc)
else
    echo "Coverity not installed. Running normal build as fallback..."
    cmake ..
    make -j$(nproc)
fi

echo "===== Build Completed ====="
