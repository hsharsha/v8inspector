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
CXFLAGS= -DSTANDALONE_BUILD=1 -std=c++11 -ggdb3 -fno-pie -fno-inline -fPIC -shared # -O3 -Wall


LDFLAGS= -lz -lcrypto -lrt -L/usr/local/lib/ -luv -L$(HOME)/.cbdepscache/lib -lv8  -lv8_libplatform -lv8_libbase -licui18n -licuuc
SOURCES=http_parser.cc inspector_socket.cc inspector_socket_server.cc inspector_io.cc inspector_agent.cc

INCLUDE_DIRS=-I. -I/$(HOME)/.cbdepscache/include
OUT=libinspector.so

build:
	python compress_json.py js_protocol.json v8_inspector_protocol_json.h
	$(CXX) $(CXFLAGS) $(SOURCES) $(INCLUDE_DIRS) -o $(OUT)
	$(CXX) -std=c++11 -ggdb3 $(INCLUDE_DIRS) main.cc -L. -linspector $(LDFLAGS) -o inspector

allopt:
	python compress_json.py js_protocol.json v8_inspector_protocol_json.h
	$(CXX) $(CXFLAGS) $(SOURCES) $(INCLUDE_DIRS) -O3 -o $(OUT)
	$(CXX) -std=c++11 -ggdb3 $(INCLUDE_DIRS) main.cc -L. -linspector $(LDFLAGS) -O3 -o inspector

clean:
	-rm -rf $(OUT)
