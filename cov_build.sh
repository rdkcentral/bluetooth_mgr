#!/bin/bash
set -euo pipefail

WORKSPACE="$(pwd)"
BTMGR_SRC="${WORKSPACE}/src/btMgr"
PREFIX_PATH="${WORKSPACE}/local"

if [ ! -d "${BTMGR_SRC}" ]; then
  echo "Error: bluetooth_mgr source not found at ${BTMGR_SRC}"
  exit 1
fi

mkdir -p "${PREFIX_PATH}"

cd "${BTMGR_SRC}"

echo "Bootstrapping autotools..."

if [ ! -f config.h.in ]; then
  cat > config.h.in <<'EOF'
/* Stub config.h.in for CI native build */
EOF
fi

libtoolize --force
aclocal
automake --force-missing --add-missing
autoconf

echo "Configuring project..."

export CPPFLAGS="-I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I${PREFIX_PATH}/include -I${PREFIX_PATH}/include/cjson -I/usr/include/cjson"
export LDFLAGS="-L${PREFIX_PATH}/lib"
export LIBCJSON_CFLAGS="-I${PREFIX_PATH}/include/cjson -I/usr/include/cjson"
export LIBCJSON_LIBS="-lcjson"
export CFLAGS="-Wno-error"
export CXXFLAGS="-Wno-error"
export LD_LIBRARY_PATH="${PREFIX_PATH}/lib:${LD_LIBRARY_PATH:-}"
export PKG_CONFIG_PATH="${PREFIX_PATH}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

./configure \
  --prefix="${PREFIX_PATH}" \
  --enable-gstreamer1=yes \
  --enable-pi-build=yes \
  --enable-autoconnectfeature=yes \
  --enable-safec=no \
  --enable-rdk-logger=no

echo "Building Bluetooth Manager components..."

make -j"$(nproc)" V=1

echo "Build completed successfully."
