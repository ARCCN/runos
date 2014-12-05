#!/bin/sh -e

cd $(dirname $0)

cd libfluid_base
git clean -fdx
cd ..

cd libfluid_msg
git clean -fdx
cd ..

cd libtins
git clean -fdx
cd ..
