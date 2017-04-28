# Copyright (c) 2017 Couchbase, Inc.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS"
# BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
# or implied. See the License for the specific language governing
# permissions and limitations under the License.

CXX=g++
CXFLAGS= -std=c++11 -ggdb3 -fno-pie -fno-inline # -O3 -Wall


LDFLAGS=-luv -L/usr/local/opt/zlib/lib -lz -L/usr/local/opt/openssl/lib -lcrypto -L/Users/$(USER)/dev/v8/out/x64.release/ -lv8 -lv8_libplatform -lv8_libbase -L/Users/$(USER)/dev/v8/samples_1/ -licudata -licui18n -licustubdata -licuucx
SOURCES=shell.cc http_parser.c inspector_socket.cc inspector_socket_server.cc inspector_agent.cc

INCLUDE_DIRS=-I. -I/Users/$(USER)/dev/v8/ -I/Users/$(USER)/dev/v8/include/ -I/usr/local/opt/openssl/include -I/usr/local/opt/zlib/include -I/Users/$(USER)/dev/node/deps/icu-small/source/common/

OUT=inspector

build:
	$(CXX) $(CXFLAGS) $(SOURCES) $(INCLUDE_DIRS) $(LDFLAGS) -o $(OUT)

allopt:
	$(CXX) $(CXFLAGS) $(SOURCES) $(INCLUDE_DIRS) $(LDFLAGS) -O3 -o $(OUT)


clean:
	-rm -rf $(OUT)
