#!/bin/bash

make clean
mkdir -vp build
bear --output build/compile_commands.json make
