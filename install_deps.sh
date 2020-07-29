#!/bin/bash

set -x

gtest_version=v1.10.x
gtest_url=https://github.com/google/googletest/archive/${gtest_version}.zip
current_dir=`pwd`
install_root=${current_dir}/carbin
download_root=${install_root}/download
echo ${install_root}

mkdir -p ${download_root}
cd ${download_root}

wget ${gtest_url}
unzip ${gtest_version}.zip
cd googletest-1.10.x
cmake . -DCMAKE_INSTALL_PREFIX=${install_root}
make install

cd ${download_root}
benchmark_url=https://github.com/gottingen/benchmark/archive/dev.zip
wget ${benchmark_url}
unzip dev.zip
cd benchmark-dev
cmake . -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DBENCHMARK_ENABLE_TESTING=OFF  -DCMAKE_INSTALL_PREFIX=${install_root}
make install