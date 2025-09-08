#!/bin/bash

mkdir -p bin

g++ main.cpp \
  -o bin/shanat \
  -std=c++11 -I/usr/include/drm \
  -lEGL -lGLESv2 -lgbm -ldrm -lpthread -lm
