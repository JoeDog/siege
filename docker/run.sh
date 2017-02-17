#!/usr/bin/env bash

rm -rf build
mkdir build

docker run --rm -t -v $PWD/build:/opt/siege -v $PWD/compile.sh:/opt/compile.sh ubuntu bash /opt/compile.sh
docker build . -t jstarcher/siege:latest
