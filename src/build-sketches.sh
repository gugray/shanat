#!/bin/bash

mkdir -p bin

./shader-includes.sh shanat-sketches/lissaj

cd shanat-sketches

g++ main.cpp fps.cpp geo.cpp horrors.cpp \
  lissaj/lissaj.cpp \
  -o ../bin/shanat \
  -std=c++11 \
  -I/usr/include/drm -I/usr/include/libdrm \
  -lEGL -lGLESv2 -lgbm -ldrm -lpthread -lm

cd ..

