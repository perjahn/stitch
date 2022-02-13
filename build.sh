#!/bin/bash

set -e

clang stitch.c -lm -o stitch
# ./stitch 3 3 input*.bmp output.bmp
