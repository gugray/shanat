#!/bin/bash

mkdir -p ../bin

g++ shanat-live/main.cpp shanat-live/argparse.cpp \
    -o ../bin/shanat-arg \
    -std=c++11 \

