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

## 3rd Partdy Dependencies / Windows libraries:
libuv
download from https://dist.libuv.org/dist/v1.10.0/
rename to .zip and unpack at v8Inspector\libuv_1_10\x86\
includes prebuild binaries

icu
Download from https://github.com/unicode-org/icu/releases/tag/release-59-2
Install at v8Inspector\icu
Includes a Visual Studio Solution at v8Inspector\icu\source\allinone\allinone.sln
This required installing some updates for VC2017. 

lz4
Download project from https://github.com/lz4/lz4
Unpack at v8Inspector\lz4-dev
Includes a VC2017 solution at v8Inspector\lz4-dev\visual\VS2017\lz4.sln

openssl
Donload binary distribution at https://kb.firedaemon.com/support/solutions/articles/4000121705
Unpack to v8Inspector\openssl

zlib
Download from https://www.nuget.org/packages/zlib128-vc140-static-32_64/
Rename to zip and Unpack at D:\Develop\google_v8\v8Inspector\zlib\
