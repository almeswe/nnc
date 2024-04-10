#!/bin/sh

./nnc $@ 2> bootstrap.s 
gcc -g -c bootstrap.s -o compiled.o
ld compiled.o -o compiled 