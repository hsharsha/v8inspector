# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS"
# BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
# or implied. See the License for the specific language governing
# permissions and limitations under the License.

# Locate libuv library
# This module defines
#  LIBUV_FOUND, if false, do not try to link with libuv
#  LIBUV_LIBRARIES, Library path and libs
#  LIBUV_INCLUDE_DIR, where to find the ICU headers

SET(LIBUV_INCLUDE_DIR "D:/Develop/google_v8/v8Inspector/libuv_1_10/include")

SET(LIBUV_LIBRARIES "D:/Develop/google_v8/v8Inspector/libuv_1_10/libuv.lib")


### FIND_PATH(LIBUV_INCLUDE_DIR uv.h
###           PATH_SUFFIXES include)
### 
### FIND_LIBRARY(LIBUV_LIBRARIES
###           NAMES uv libuv
###           PATH_SUFFIXES lib)
### 
IF (LIBUV_LIBRARIES)
  MESSAGE(STATUS "Found libuv headers in ${LIBUV_INCLUDE_DIR}")
  MESSAGE(STATUS "Using libuv libraries ${LIBUV_LIBRARIES}")
ELSE (LIBUV_LIBRARIES)
  MESSAGE(FATAL_ERROR "Can't build v8inspector without libuv")
ENDIF (LIBUV_LIBRARIES)

MARK_AS_ADVANCED(LIBUV_INCLUDE_DIR LIBUV_LIBRARIES)
