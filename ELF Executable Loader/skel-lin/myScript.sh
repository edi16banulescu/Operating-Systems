#!/bin/bash

make && cp libso_loader.so ../checker-lin && cd ../checker-lin &&  make -f Makefile.checker && cd ../skel-lin
