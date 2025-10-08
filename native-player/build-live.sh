#!/bin/bash

mkdir -p ../bin

g++ shanat-live/main.cpp shanat-live/hot_file.cpp shanat-live/arg_parse.cpp \
    shanat-shared/fps.cpp shanat-shared/geo.cpp shanat-shared/horrors.cpp \
    -o ../bin/shanat-live \
    -std=c++11 \
    -I/usr/include/drm -I/usr/include/libdrm \
    -lEGL -lGLESv2 -lgbm -ldrm -lpthread -lm

