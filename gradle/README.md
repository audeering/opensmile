# Gradle build for android

to build the libraries you can set the flags and parameters as in the following example:

```(bash)
gradle assembleRelease \
-PabiFilters="arm64-v8a armeabi-v7a x86_64" \
-PBUILD_FLAGS="-DBUILD_LIBSVM -DBUILD_RNN -DBUILD_SVMSMO" \
-PCMAKE_FLAGS="-DWITH_OPENSLES=ON" \
-PdoNotStrip=false

```

The assemble parameter can be `assemble`, `assembleRelease`, or `assembleDebug`.

In the `gradle.properties` the default values are set.
