# Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
#
# Makefile for FastBit collection server.

# Compiler
CC=g++
# Libraries
LDFLAGS=-lzmq -lprotobuf -lfastbit
LDFLAGS_PATH=$(LDFLAGS) -I/opt/local/include -L/opt/local/lib/amd64 -L/opt/local/lib

.PHONY: rel

all: listener

listener: pckt
	$(CC) listener.cc pckt.pb.cc $(LDFLAGS_PATH) -o listener

pckt:
	protoc -I=. --cpp_out=. ./pckt.proto

csv_util:
	$(CC) make_csv.cc $(LDFLAGS_PATH) -o make_csv
	$(CC) combine_csv.cc $(LDFLAGS_PATH) -o combine_csv

rel: listener
	cp ./listener ./bin/

clean:
	-rm listener
	-rm pckt.pb.h
	-rm pckt.pb.cc

