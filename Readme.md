## Introduction

This is a standalone integration of v8/v8-inspector with the debugger of chrome-devtools, based on the work of hsharsha in his parent project at https://github.com/hsharsha/v8inspector

I added support for Windows x64/x86 and made some improvements etc.
Although this project originates from a different platform i only made and tested windows builds.
cmake files originated from other platforms still included and still should be valid (no source files added).

## Build Instructions
After a successful build it should only be necessary to start "inspector-demo" (main.cc) in Visual Studio.
Then open the URL printed to the console in chrome.

## Known problems and limitations
* chrome sometimes seems to send in one debug session messages to a inspector object of a already deleted previous session. This seems to be handled now robust in the implementation by checking for a magic number.

* The program crashed in the v8 libraries when the global scope was opened in the debugger. I tracked this down to a call containing the flag "ownProperties":true and worked around this by not dispatching such calls.
I dont know if this is caused by the update to 7.1. 

* The original project referenced a include file v8_inspector_protocol_json.h which was not included.
The only location where this is used (SendProtocolJson) seems never to be called. I commented this out and added a warning in case its called.
    


## Windows Build
Project Files/Solution for VC2017 included using v8 Version 7.1.302.4
See below for 3rd party libraries. 

## 3rd Party Dependencies / Windows libraries:
libuv<br>
download from https://dist.libuv.org/dist/v1.10.0/<br>
rename to .zip and unpack at google_v8\v8Inspector\libuv_1_10\x86\ <br>
includes prebuild binaries<br>

icu<br>
Download from https://github.com/unicode-org/icu/releases/tag/release-59-2<br>
Install at v8Inspector\icu<br>
Includes a Visual Studio Solution at google_v8\v8Inspector\icu\source\allinone\allinone.sln<br>
This required installing some updates for VC2017. <br>

lz4<br>
Download project from https://github.com/lz4/lz4<br>
Unpack at v8Inspector\lz4-dev<br>
Includes a VC2017 solution at google_v8\v8Inspector\lz4-dev\visual\VS2017\lz4.sln<br>

openssl<br>
Donwload binary distribution at https://kb.firedaemon.com/support/solutions/articles/4000121705<br>
Unpack to google_v8\v8Inspector\openssl<br>

zlib<br>
Download from https://www.nuget.org/packages/zlib128-vc140-static-32_64/<br>
Rename to zip and Unpack at google_v8\v8Inspector\zlib\<br>

v8 libraries<br>
Download prebuild from <br>
https://www.nuget.org/packages/v8-v140-x64/7.1.302.4<br>
https://www.nuget.org/packages/v8-v140-x86/7.1.302.4<br>
https://www.nuget.org/packages/v8.redist-v140-x64/7.1.302.4<br>
https://www.nuget.org/packages/v8.redist-v140-x86/7.1.302.4<br>
and unpack to google_v8 dir<br>


Your directory structure should now look like this:
```shell
google_v8\v8-v141-x64.7.1.302.4
google_v8\v8-v141-x86.7.1.302.4
google_v8\v8Inspector\icu
google_v8\v8Inspector\libuv_1_10
google_v8\v8Inspector\lz4-dev
google_v8\v8Inspector\openssl
google_v8\v8Inspector\v8inspector
google_v8\v8Inspector\zlib
```

I had to copy the following files from v8.redist-v140-x64/7.1.302.4 into the build directory v8Inspector\v8inspector\build\Debugx64\ order to make things run:
```shell
natives_blob.bin
snapshot_blob.bin
icudtl.dat
icui18n.dll
icuuc.dll
v8.dll
v8_libbase.dll
v8_libplatform.dll
```
