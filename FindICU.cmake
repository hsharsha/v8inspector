# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS"
# BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
# or implied. See the License for the specific language governing
# permissions and limitations under the License.

# Locate icu4c library
# This module defines
#  ICU_FOUND, if false, do not try to link with ICU
#  ICU_LIBRARIES, Library path and libs
#  ICU_INCLUDE_DIR, where to find the ICU headers

SET(ICU_INCLUDE_DIR "D:/Develop/google_v8/v8Inspector/icu/include/unicode/")

FIND_PATH(ICU_INCLUDE_DIR unicode/utypes.h
          PATH_SUFFIXES include)

IF (ICU_INCLUDE_DIR)
  STRING(STRIP ${ICU_INCLUDE_DIR} ICU_INCLUDE_DIR)
  STRING(STRIP "${ICU_LIB_HINT_DIR}" ICU_LIB_HINT_DIR)

  IF (NOT ICU_LIBRARIES)
      SET(_icu_libraries "icuuc;icui18n")
      FOREACH(_mylib ${_icu_libraries})
         UNSET(_the_lib CACHE)
         FIND_LIBRARY(_the_lib
                      NAMES ${_mylib}
                      HINTS
                          ${ICU_LIB_HINT_DIR}
                          PATH_SUFFIXES lib)
         IF (_the_lib)
             LIST(APPEND ICU_LIBRARIES ${_the_lib})
         ENDIF (_the_lib)
      ENDFOREACH(_mylib)
  ENDIF(NOT ICU_LIBRARIES)

  MESSAGE(STATUS "Found ICU headers in ${ICU_INCLUDE_DIR}")
  MESSAGE(STATUS "Using ICU libraries: ${ICU_LIBRARIES}")
ELSE (ICU_INCLUDE_DIR)
  MESSAGE(FATAL_ERROR "Can't build v8inspector without ICU")
ENDIF (ICU_INCLUDE_DIR)

MARK_AS_ADVANCED(ICU_INCLUDE_DIR ICU_LIBRARIES)
