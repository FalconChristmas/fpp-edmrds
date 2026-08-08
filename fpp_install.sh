#!/bin/bash
set -e

# fpp-edmrds install script
#
# Builds the C++ plugin. No pigpio (gone on FPP10/trixie) and no other
# packages: the I2C bit-bang runs through FPP's PinCapabilities GPIO layer.

BASEDIR=$(dirname $0)
cd $BASEDIR
make "SRCDIR=${SRCDIR}"

# No restartFlag: the plugin declares FPP_PLUGIN_SUPPORTS_UNLOAD and the Plugin
# Manager asks fppd to load it as soon as this script finishes, so asking the
# user to restart would interrupt a running show for nothing.
