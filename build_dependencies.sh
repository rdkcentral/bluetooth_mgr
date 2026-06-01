#!/bin/bash
set -euo pipefail

WORKSPACE="$(pwd)"
PREFIX_PATH="${WORKSPACE}/local"
TARGET_BRANCH="${TARGET_BRANCH:-develop}"

echo "Installing system dependencies..."
sudo apt-get update
sudo apt-get install -y \
  git \
  build-essential \
  gcc \
  g++ \
  make \
  pkg-config \
  m4 \
  autoconf \
  automake \
  libtool \
  libglib2.0-dev \
  libdbus-1-dev \
  libudev-dev \
  libbluetooth-dev \
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

mkdir -p "${PREFIX_PATH}/include/bluetooth/audio"
mkdir -p "${PREFIX_PATH}/lib"

echo "Cloning bluetooth (btcore provider)..."
rm -rf "${WORKSPACE}/src/btrcore"
git clone https://github.com/rdkcentral/bluetooth.git "${WORKSPACE}/src/btrcore"

if git -C "${WORKSPACE}/src/btrcore" rev-parse --verify "origin/${TARGET_BRANCH}" >/dev/null 2>&1; then
  git -C "${WORKSPACE}/src/btrcore" checkout "${TARGET_BRANCH}"
else
  echo "Branch ${TARGET_BRANCH} not found in rdkcentral/bluetooth; using repository default branch checkout."
fi

echo "Fetching BlueZ legacy headers..."
rm -rf "${WORKSPACE}/bluez"
git clone https://github.com/bluez/bluez.git "${WORKSPACE}/bluez"
git -C "${WORKSPACE}/bluez" checkout tags/5.48

cp "${WORKSPACE}/bluez/profiles/audio/a2dp-codecs.h" \
   "${PREFIX_PATH}/include/bluetooth/audio/a2dp-codecs.h"

cp "${WORKSPACE}/bluez/lib/bluetooth.h" \
   "${PREFIX_PATH}/include/bluetooth/bluetooth.h"

echo "Building and installing btcore..."
pushd "${WORKSPACE}/src/btrcore" >/dev/null

libtoolize --force
aclocal
autoheader
automake --force-missing --add-missing
autoconf
autoreconf --install -f

export CPPFLAGS="-I${PREFIX_PATH}/include -Wno-error=unused-result -Wno-error=stringop-truncation"
export LDFLAGS="-L${PREFIX_PATH}/lib"
export CFLAGS="-Wno-error"
export CXXFLAGS="-Wno-error"

./configure \
  --prefix="${PREFIX_PATH}" \
  --enable-btr-ifce=bluez5 \
  --enable-telemetry=no

make -j"$(nproc)" V=1
make install

popd >/dev/null

echo "Dependency setup completed successfully."
