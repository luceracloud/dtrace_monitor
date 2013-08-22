# Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
#
# Makefile for statistics generator/server

# Compiler
CC=g++
# Libraries
LDFLAGS=-ldtrace -lzmq -lkstat -lprotobuf
LDFLAGS_PATH=$(LDFLAGS) -I/opt/local/include -L/opt/local/lib/amd64 -L/opt/local/lib

.PHONY: rel

# Instructions
all: server_release

server_release: server.cpp scripts.hpp pckt
	$(CC) server.cpp pckt.pb.cc $(LDFLAGS_PATH) -O2 -o generator

server_release_zone: server.cpp scripts.hpp pckt
	$(CC) server.cpp -DZONE pckt.pb.cc $(LDFLAGS_PATH) -O2 -o generator_zone

pckt:
	protoc -I=. --cpp_out=. ./pckt.proto

rel: server_release server_release_zone
	cp generator metric/bin
	cp generator_zone metric/bin

clean:
	-rm pckt.pb.cc
	-rm pckt.pb.h
	-rm generator
	-rm generator_zone
	-rm metric/bin/generator
	-rm metric/bin/generator_zone
