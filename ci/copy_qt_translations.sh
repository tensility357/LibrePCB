#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Copy Qt translation files to output directory since linuxdeployqt doesn't
# do this (see https://github.com/probonopd/linuxdeployqt/issues/115).
SRC=`qmake -query QT_INSTALL_TRANSLATIONS`/qt*.qm
DST="./build/install/translations/"
mkdir "$DST"
cp -f $SRC $DST
