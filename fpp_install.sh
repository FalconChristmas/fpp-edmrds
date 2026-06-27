#!/bin/bash

# fpp-edmrds install script
#
# Builds the C++ plugin. No pigpio (gone on FPP10/trixie) and no other
# packages: the I2C bit-bang runs through FPP's PinCapabilities GPIO layer.

BASEDIR=$(dirname $0)
cd $BASEDIR
make "SRCDIR=${SRCDIR}"

. ${FPPDIR}/scripts/common
setSetting restartFlag 1
