#!/bin/sh
# OpenSSL build script for android
# Author:	Max.Chiu

# Config android enviroment
. ../setenv-android.sh

# Config openssl for android
./config no-ssl2 no-ssl3 no-comp no-hw no-engine --openssldir=$ANDROID_NDK_ROOT/platforms/$ANDROID_API/$ANDROID_ARCH/usr

# build
make clean

make depend || exit 1

make

#make install
