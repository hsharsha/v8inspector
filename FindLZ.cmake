# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS"
# BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
# or implied. See the License for the specific language governing
# permissions and limitations under the License.

# Locate lz4 library
# This module defines
#  LZ4_FOUND, if false, do not try to link with lz4
#  LZ4_LIBRARIES, Library path and libs
#  LZ4_INCLUDE_DIR, where to find the ICU headers


## FIND_LIBRARY(LZ_LIBRARIES
##              NAMES z
##              PATH_SUFFIXES lib)
## 
SET(LZ_LIBRARIES "D:/Develop/google_v8/v8Inspector/lz4-dev/visual/VS2017/bin/x64_Debug/liblz4.lib")

IF (LZ_LIBRARIES)
  MESSAGE(STATUS "Using LZ library: ${LZ_LIBRARIES}")
ELSE(LZ_LIBRARIES)
    MESSAGE(FATAL_ERROR "LZ library not found needed for v8inspector")
ENDIF(LZ_LIBRARIES)

MARK_AS_ADVANCED(LZ_LIBRARIES)
