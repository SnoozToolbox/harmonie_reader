#!/bin/bash

cd build
make
cd ..

cp build/HarmonieReader.cpython-310-darwin.so /Users/davidlevesque/projects/scinodes_poc/src/main/resources/mac/extension/PSGReader/HarmonieReader.so
