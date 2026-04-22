#!/bin/bash

set -e

echo "===== Installing bluetooth_mgr dependencies ====="

# Update system
sudo apt-get update

# Install core dependencies
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    wget \
    libglib2.0-dev \
    libbluetooth-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    python3 \
    python3-pip \
    lcov

# (Optional) Install RDK specific dependencies if required
# You can extend this section depending on bluetooth_mgr requirements

echo "===== Dependencies installed successfully ====="
