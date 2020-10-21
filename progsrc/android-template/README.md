# Building the Android sample app

First, in order to build the Java SWIG files, install swig first, and then run:
```bash
./gradlew opensmile:swig
```
This has been tested with SWIG Version 4.0.1.

To build the aar libraries, set the openSMILE parameters in opensmile/build.gradle, then run:
```bash
./gradlew opensmile:assemble
```

Finally, if the aar file is already added to the libs folder, run the following to build the app:
```bash
./gradlew app:assembleUseAar
```
Otherwise, to build the openSMILE libraries from scratch, run:
```bash
./gradlew app:assembleUseSource
```
