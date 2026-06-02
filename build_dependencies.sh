#!/bin/bash
set -euo pipefail

echo "Installing system dependencies..."

sudo apt-get update
sudo apt-get install -y \
  autoconf automake libtool pkg-config \
  gcc g++ make \
  git \
  libglib2.0-dev \
  libdbus-1-dev \
  libbluetooth-dev \
  libudev-dev \
  libcjson-dev \
  libunwind-dev \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  libgstreamer-plugins-good1.0-dev \
  libgstreamer-plugins-bad1.0-dev \
  gstreamer1.0-tools \
  libcurl4-openssl-dev \
  gobject-introspection \
  libgirepository1.0-dev \
  bluez \
  libcairo2-dev

WORKSPACE="$(pwd)"
BTMGR_SRC="${WORKSPACE}/src/btMgr"

mkdir -p "${WORKSPACE}/src"

echo "Cloning bluetooth_mgr source..."
rm -rf "${BTMGR_SRC}"
git clone https://github.com/rdkcentral/bluetooth_mgr.git "${BTMGR_SRC}"

echo "Dependencies setup completed."
