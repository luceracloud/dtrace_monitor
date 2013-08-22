#!/usr/bin/env bash
BASE=${PWD}/..

LD_LIBRARY_PATH=$BASE/bin/lib $BASE/bin/listener -a $1

