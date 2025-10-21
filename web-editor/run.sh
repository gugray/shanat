#!/bin/bash

BASEDIR=/home/gabor/shader-live
export SHADER_DIR=$BASEDIR
export STORAGE_DIR=$BASEDIR/data

nohup node src-server/run.js >$BASEDIR/data/server.log  2>&1 &
