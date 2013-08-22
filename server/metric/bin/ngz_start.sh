#!/usr/bin/env bash
# Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
BASE=${PWD}/../

LD_LIBRARY_PATH=$BASE/lib $BASE/bin/generator_zone $1
