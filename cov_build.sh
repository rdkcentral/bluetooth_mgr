#!/bin/bash
set -euo pipefail

WORKSPACE="$(pwd)"
PREFIX_PATH="${WORKSPACE}/local"

echo "Bootstrapping bluetooth_mgr autotools..."

if [ ! -f config.h.in ]; then
  cat > config.h.in <<'EOF'
/* Stub config.h.in for CI native build */
EOF
fi

libtoolize --force
aclocal
automake --force-missing --add-missing
autoconf
autoreconf --install -f

echo "Configuring bluetooth_mgr..."

export PREFIX_PATH
export CPPFLAGS="-I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I${PREFIX_PATH}/include -I${PREFIX_PATH}/include/cjson -I/usr/include/cjson"
export LDFLAGS="-L${PREFIX_PATH}/lib -lbtrCore"
export LIBCJSON_CFLAGS="-I${PREFIX_PATH}/include/cjson -I/usr/include/cjson"
export LIBCJSON_LIBS="-lcjson"
export CFLAGS="-Wno-error"
export CXXFLAGS="-Wno-error"
export PKG_CONFIG_PATH="${PREFIX_PATH}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export LD_LIBRARY_PATH="${PREFIX_PATH}/lib:${LD_LIBRARY_PATH:-}"

./configure \
  --prefix="${PREFIX_PATH}" \
  --enable-gstreamer1=yes \
  --enable-pi-build=yes \
  --enable-autoconnectfeature=yes \
  --enable-safec=no \
  --enable-rdk-logger=no

echo "Building bluetooth_mgr..."
make -j"$(nproc)" V=1

echo "Installing bluetooth_mgr..."
make install

echo "Build completed successfully."
