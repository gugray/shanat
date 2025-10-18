#!/bin/bash

mkdir -p ../bin

./shader-includes.sh shanat-sketches/lissaj

g++ shanat-sketches/main.cpp shanat-sketches/lissaj/lissaj.cpp \
    shanat-shared/fps.cpp shanat-shared/geo.cpp shanat-shared/horrors.cpp \
    -o ../bin/shanat-sketches \
    -std=c++11 \
    -I/usr/include/drm -I/usr/include/libdrm \
    -lEGL -lGLESv2 -lgbm -ldrm -lpthread -lm

