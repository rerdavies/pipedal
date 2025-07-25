#!/usr/bin/env bash
# Check for required system packages for building PiPedal.

set -e

packages=(\
    liblilv-dev libboost-dev libsystemd-dev catch libasound2-dev uuid-dev \
    authbind libavahi-client-dev libnm-dev libicu-dev \
    libsdbus-c++-dev libzip-dev google-perftools \
    libgoogle-perftools-dev libpipewire-0.3-dev \
    libwebsocketpp-dev
)

missing=()

for pkg in "${packages[@]}"; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        missing+=("$pkg")
    fi
done

if [ ${#missing[@]} -ne 0 ]; then
    echo "Missing packages:" >&2
    for pkg in "${missing[@]}"; do
        echo "  $pkg" >&2
    done
    exit 1
else
    echo "All required packages are installed." >&2
fi