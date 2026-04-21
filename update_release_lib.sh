#!/bin/bash

cd build
make
cd ..

cp build/HarmonieReader.cpython-310-x86_64-linux-gnu.so /home/david/projects/scinodes_poc/src/main/resources/linux/extension/PSGReader/HarmonieReader.so
 