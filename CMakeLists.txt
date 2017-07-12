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

CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
IF (${CMAKE_MAJOR_VERSION} GREATER 2)
    CMAKE_POLICY(SET CMP0042 NEW)
ENDIF (${CMAKE_MAJOR_VERSION} GREATER 2)


PROJECT (v8inspector)

INCLUDE (FindCouchbaseEventingDep)
INCLUDE (FindCouchbaseOpenSSL)

INCLUDE_DIRECTORIES(BEFORE ${LIBUV_INCLUDE_DIR})

INCLUDE_DIRECTORIES(AFTER  ${ICU_INCLUDE_DIR}
                           ${V8_INCLUDE_DIR}
                           ${OPENSSL_INCLUDE_DIR}
                           ${V8_INCLUDE_DIR}/include
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${Platform_SOURCE_DIR}/include
                           ${CMAKE_INSTALL_PREFIX}/include)

EXECUTE_PROCESS(COMMAND python compress_json.py js_protocol.json v8_inspector_protocol_json.h
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

SET(V8INSPECTOR_SOURCES assert.cc http_parser.cc inspector_agent.cc
    inspector_io.cc inspector_socket.cc inspector_socket_server.cc)
SET(V8INSPECTOR_LIBRARIES ${V8_LIBRARIES} ${ICU_LIBRARIES} -lz ${LIBUV_LIBRARIES} platform ${OPENSSL_CRYPT_LIBRARY})
ADD_LIBRARY(v8inspector SHARED ${V8INSPECTOR_SOURCES})
SET_TARGET_PROPERTIES(v8inspector PROPERTIES POSITION_INDEPENDENT_CODE true)
TARGET_LINK_LIBRARIES(v8inspector ${V8INSPECTOR_LIBRARIES})
INSTALL(TARGETS v8inspector
        LIBRARY DESTINATION lib)
