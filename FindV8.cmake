# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS"
# BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
# or implied. See the License for the specific language governing
# permissions and limitations under the License.

# Locate v8 library
# This module defines
#  V8_FOUND, if false, do not try to link with v8
#  V8_LIBRARIES, Library path and libs
#  V8_INCLUDE_DIR, where to find V8 headers

### FIND_PATH(V8_INCLUDE_DIR v8.h
###           PATH_SUFFIXES include)
SET(V8_INCLUDE_DIR "D:/Develop/google_v8/v8-v141-x64.7.1.302.4/include")

### FIND_LIBRARY(V8_SHAREDLIB
###              NAMES v8
###              PATH_SUFFIXES lib)
### 
### FIND_LIBRARY(V8_PLATFORMLIB
###              NAMES v8_libplatform
###              PATH_SUFFIXES lib)
### 
### FIND_LIBRARY(V8_BASELIB
###              NAMES v8_libbase
###              PATH_SUFFIXES lib)






SET(V8_SHAREDLIB   "D:/Develop/google_v8/v8-v141-x64.7.1.302.4/lib/Release/v8.dll.lib") 
SET(V8_PLATFORMLIB "D:/Develop/google_v8/v8-v141-x64.7.1.302.4/lib/Release/v8_libbase.dll.lib") 
SET(V8_BASELIB     "D:/Develop/google_v8/v8-v141-x64.7.1.302.4/lib/Release/v8_libplatform.dll.lib") 

IF (V8_SHAREDLIB AND V8_PLATFORMLIB AND V8_BASELIB)
    SET(V8_LIBRARIES ${V8_SHAREDLIB} ${V8_PLATFORMLIB} ${V8_BASELIB})
ENDIF (V8_SHAREDLIB AND V8_PLATFORMLIB AND V8_BASELIB)

IF (V8_LIBRARIES)
  MESSAGE(STATUS "Found v8 headers in ${V8_INCLUDE_DIR}")
  MESSAGE(STATUS "Using v8 libraries ${V8_LIBRARIES}")
ELSE (V8_LIBRARIES)
  MESSAGE(FATAL_ERROR "Can't build v8inspector without V8")
ENDIF (V8_LIBRARIES)

MARK_AS_ADVANCED(V8_INCLUDE_DIR V8_LIBRARIES)
