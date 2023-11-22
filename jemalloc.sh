#!/usr/bin/bash
#####################################
#make jemalloc
#####################################

crtdir=$(pwd)
libdir=${crtdir}/Build/Linux/commonlib
mkdir -p ${libdir}

if [ ! -f ${libdir}/libjemalloc.so ];then
    cd ./third_include/jemalloc
    ./autogen.sh
    ./configure
    make
    cp lib/libjemalloc.so.2 ${libdir}/libjemalloc.so
    make clean
    cd ${crtdir}
fi
