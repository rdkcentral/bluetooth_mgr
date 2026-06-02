#!/bin/bash
set -e

ROOT=$(pwd)

# ========================================================
# STEP 1: Build BTRCore
# ========================================================
echo "==== Cloning bluetooth repo ===="

if [ ! -d bluetooth ]; then
    git clone https://github.com/rdkcentral/bluetooth.git
fi

cd bluetooth

echo "==== Bootstrapping autotools ===="
libtoolize --force
aclocal
autoheader
automake --add-missing --force-missing
autoconf

echo "==== Setting environment ===="

export CPPFLAGS="-I${ROOT}/external/include"
export LDFLAGS="-L${ROOT}/external/lib"
export LD_LIBRARY_PATH="${ROOT}/external/lib:$LD_LIBRARY_PATH"
export CFLAGS="-Wno-error"
export CXXFLAGS="-Wno-error"

echo "==== Running configure (BTRCore) ===="

ac_cv_header_telemetry_busmessage_sender_h=yes \
./configure --prefix=/usr

echo "==== Building BTRCore ===="
make -C src/bt-ifce -j$(nproc)
make -C src -j$(nproc)


echo "==== Installing BTRCore ===="
sudo make install
sudo ldconfig

echo "==== Verifying libbtrCore ===="
ldconfig -p | grep btrCore || true

cd ${ROOT}

# ========================================================
# STEP 2: Build bluetooth_mgr
# ========================================================
echo "==== Preparing bluetooth_mgr ===="

chmod +x autogen.sh || true
./autogen.sh || autoreconf -i

echo "==== Configuring bluetooth_mgr ===="

export CPPFLAGS="-I${ROOT}/external/include"
export LDFLAGS="-L${ROOT}/external/lib -lbtrCore -ltelemetry_msgsender"

ac_cv_header_telemetry_busmessage_sender_h=yes \
ac_cv_lib_btrCore_BtrCore_Init=yes \
ac_cv_header_btrCore_h=yes \
./configure --prefix=/usr

echo "==== Building bluetooth_mgr ===="
make -j$(nproc)

echo "bluetooth_mgr build completed successfully"
