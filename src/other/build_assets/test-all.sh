#!/usr/bin/env bash

cd ..
export QUICKBLOCKS=`pwd`
export BUILD_FOLDER=$QUICKBLOCKS/build/
export TEST_FOLDER=$QUICKBLOCKS/test/

#echo "Making..."
cd $BUILD_FOLDER/
cmake ../src
cd dev_tools
make -j 8
cd ..
make generate finish
make -j 8

test-api.sh --filter all --mode both --clean --report $@

cd $BUILD_FOLDER
echo "Done..."
