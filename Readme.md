## Introduction

This is a standalone integration of v8-inspector with chrome-devtools front end, to debug any standalone java script code.

This code is inspired (read as copied and ripped apart) from node.js and v8 code base.

This was mainly done to enable debugging of [couchbase eventing functions](https://github.com/couchbase/eventing/tree/master/third_party/inspector)

## Library Dependencies
This project depends on following libraries
* V8 5.9
* ICU 59
* libuv 1.8
* libz
* libcrypto

## Build Instructions
```shell
$ mkdir build && cd build
$ cmake ..
$ make
# To run the binary
$ ./inspector ../sample.js
```

