# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS"
# BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
# or implied. See the License for the specific language governing
# permissions and limitations under the License.

# Locate OpenSSL library
# This module defines
#  OPENSSL_LIBRARIES, Library path and libs
#  OPENSSL_INCLUDE_DIR, where to find the ICU headers

SET(OPENSSL_INCLUDE_DIR "D:/Develop/google_v8/v8Inspector/openssl/openssl-1.1/x64/include/openssl/")
SET(OPENSSL_LIBRARIES   "D:/Develop/google_v8/v8Inspector/openssl/openssl-1.1/x64/lib/libcrypto.lib")

## FIND_PATH(OPENSSL_INCLUDE_DIR openssl/ssl.h
##           PATH_SUFFIXES include)
## 
## FIND_LIBRARY(OPENSSL_LIBRARIES
##              NAMES crypto
##              PATH_SUFFIXES lib)
## 
IF (OPENSSL_LIBRARIES AND OPENSSL_INCLUDE_DIR)
    MESSAGE(STATUS "Found OpenSSL headers in ${OPENSSL_INCLUDE_DIR}")
    MESSAGE(STATUS "Using OpenSSL libraries: ${OPENSSL_LIBRARIES}")
ELSE (OPENSSL_LIBRARIES AND OPENSSL_INCLUDE_DIR)
  MESSAGE(FATAL_ERROR "Can't build v8inspector without openssl")
ENDIF (OPENSSL_LIBRARIES AND OPENSSL_INCLUDE_DIR)

MARK_AS_ADVANCED(OPENSSL_INCLUDE_DIR OPENSSL_LIBRARIES)
