#!/bin/bash
start=$(date +'%s')
cd linux-6.19.9-moker
vng --kconfig
make -j$(nproc) 2> ../errors-6.19.9-moker
cd ..
#cat errors-6.19.9-moker
echo "kernel compilation took $(($(date +'%s') - $start)) seconds"
