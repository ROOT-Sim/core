#!/bin/sh

ninja reconfigure
ninja -t compdb c_COMPILER cpp_COMPILER > /dev/null
