#!/usr/bin/env bash
# Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
BASE=/opt/meter

LD_LIBRARY_PATH=$BASE/lib $BASE/bin/generator $1
