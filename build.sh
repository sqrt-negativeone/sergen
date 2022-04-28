#!/bin/bash

code="$PWD"
opts=-g
cd build > /dev/null
gcc $opts $code/code/main.c -o sergen
cd $code > /dev/null
