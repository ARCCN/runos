#!/bin/bash

cd `dirname $0`

PREFIX="$PWD/prefix"
rm -rf "$PREFIX" 2> /dev/null
mkdir -p "$PREFIX"

build_fluid() {
    pushd libfluid_base
    ./autogen.sh
    ./autogen.sh # Work around configure.ac bug
    ./configure --prefix="$PREFIX"
    make install
    popd

    pushd libfluid_msg
    ./autogen.sh
    ./configure --prefix="$PREFIX" \
        CXXFLAGS="-DIFHWADDRLEN=6" # Mac OS X compilation fix
    make install
    popd
}

build_tins() {
    pushd libtins
    mkdir -p build; cd build
    cmake \
        -DLIBTINS_ENABLE_CXX11=1 \
        -DCMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
        ..
    make install
    popd
}

build_json11() {
    mkdir -p "$PREFIX"/include
    cp json11/json11.hpp "$PREFIX"/include/
}

build_fluid
build_tins
build_json11
