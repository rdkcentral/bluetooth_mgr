#!/bin/bash
set -e

echo "==== Installing system dependencies ===="

sudo apt-get update
sudo apt-get install -y \
  autoconf automake libtool pkg-config \
  build-essential gcc g++ make \
  libglib2.0-dev libdbus-1-dev libbluetooth-dev \
  libudev-dev libreadline-dev \
  git

ROOT=$(pwd)

echo "==== Creating external directories ===="
mkdir -p ${ROOT}/external/include
mkdir -p ${ROOT}/external/lib

# ----------------------------------------------------
# TELEMETRY (stub)
# ----------------------------------------------------
echo "==== Setting up telemetry stub ===="

git clone --depth 1 https://github.com/rdkcentral/telemetry.git

cp telemetry/include/telemetry_busmessage_sender.h ${ROOT}/external/include/
cp telemetry/include/telemetry2_0.h ${ROOT}/external/include/

cat << 'EOF' > telemetry_stub.c
int t2_init(void) { return 0; }
int t2_event_s(const char* n, const char* v) { return 0; }
int t2_event_f(const char* n, float v) { return 0; }
int t2_event_d(const char* n, double v) { return 0; }
EOF

gcc -shared -fPIC telemetry_stub.c \
  -o ${ROOT}/external/lib/libtelemetry_msgsender.so

# ----------------------------------------------------
# BLUEZ LEGACY HEADERS
# ----------------------------------------------------
echo "==== Setting up BlueZ headers ===="

git clone --depth 1 https://github.com/bluez/bluez.git

mkdir -p ${ROOT}/external/include/bluetooth/audio
mkdir -p ${ROOT}/external/include/bluetooth

git -C bluez fetch --tags

git -C bluez checkout 4.101
cp bluez/audio/ipc.h ${ROOT}/external/include/bluetooth/audio/ipc.h

git -C bluez checkout 5.48
cp bluez/profiles/audio/a2dp-codecs.h \
   ${ROOT}/external/include/bluetooth/audio/a2dp-codecs.h

cp bluez/lib/bluetooth.h \
   ${ROOT}/external/include/bluetooth/bluetooth.h

sed -i '1i#include <stdbool.h>' \
  ${ROOT}/external/include/bluetooth/audio/ipc.h

echo "Dependencies setup completed"
