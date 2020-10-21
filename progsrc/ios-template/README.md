# OpenSmile template project
An iOS project to demonstrate how to integrate opensmile library to iOS app.

# Project setting
## Build Settings -> Header Search Paths
```
../../src/include
../../progsrc/include/
../../build_ios/ALL/include
```
## Build Settings -> Library Search Paths
```
../../build_ios/ALL
```

## Preprocessor Macros
```
__IOS__
```

# Building opensmile library
1. Run a command on terminal
```
bash buildIosUniversalLib.sh
```
2. Confirm the library is created in /opensmile/build_ios/ALL


