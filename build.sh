#!/bin/bash

code="$PWD"
opts=-g
if [ ! -d build ] 
then 
	mkdir build 
fi 
cd build > /dev/null
gcc $opts $code/code/main.c -o sergen
cd $code > /dev/null
