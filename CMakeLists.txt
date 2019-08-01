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

PROJECT (v8inspector)

#INCLUDE (FindV8.cmake)
#INCLUDE (FindICU.cmake)
INCLUDE (FindLZ.cmake)
#INCLUDE (FindLIBUV.cmake)
INCLUDE (FindOPENSSL.cmake)

SET(V8_INCLUDE_DIR /Users/gautham/.cbdepscache/include)
SET(V8_LIBRARIES
        /Users/gautham/.cbdepscache/lib/libv8.dylib
        /Users/gautham/.cbdepscache/lib/libv8_libplatform.dylib
        /Users/gautham/.cbdepscache/lib/libv8_libbase.dylib)

SET(ICU_LIBRARIES
        /Users/gautham/.cbdepscache/lib/libicui18n.dylib
        /Users/gautham/.cbdepscache/lib/libicuuc.dylib)

SET(LIBUV_LIBRARIES
        /Users/gautham/.cbdepscache/lib/libuv.dylib)

INCLUDE_DIRECTORIES(
                     ${V8_INCLUDE_DIR}
                     ${OPENSSL_INCLUDE_DIR}
                     ${V8_INCLUDE_DIR}/include
                     ${CMAKE_SOURCE_DIR})

EXECUTE_PROCESS(COMMAND python compress_json.py js_protocol.json v8_inspector_protocol_json.h
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
SET(CMAKE_CXX_STANDARD 11)
SET(V8INSPECTOR_SOURCES http_parser.cc inspector_agent.cc
    inspector_io.cc inspector_socket.cc inspector_socket_server.cc)
SET(V8INSPECTOR_LIBRARIES ${V8_LIBRARIES} ${ICU_LIBRARIES} ${LZ_LIBRARIES} ${LIBUV_LIBRARIES} ${OPENSSL_LIBRARIES})
ADD_LIBRARY(v8inspector SHARED ${V8INSPECTOR_SOURCES})
SET_TARGET_PROPERTIES(v8inspector PROPERTIES POSITION_INDEPENDENT_CODE true)
TARGET_LINK_LIBRARIES(v8inspector ${V8INSPECTOR_LIBRARIES})
ADD_EXECUTABLE(inspector main.cc)
TARGET_LINK_LIBRARIES(inspector v8inspector)
