#!/bin/bash

mkdir -p bin

./shader_includes.sh src/lissaj

cd src

g++ main.cpp fps.cpp geo.cpp horrors.cpp lissaj/lissaj.cpp \
  -o ../bin/shanat \
  -std=c++11 -I/usr/include/drm \
  -lEGL -lGLESv2 -lgbm -ldrm -lpthread -lm

cd ..
