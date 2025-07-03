#!/bin/bash

make clean
mkdir -vp build
bear --output compile_commands.json -- make
make debug
