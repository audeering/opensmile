# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [3.0.0] - 2020-10-20
## Added
- New components:
  - cDataPrintSink
  - cFunctionalModulation (re-added from version 2.2)
  - cFFmpegSource
  - cExternalSource
  - cExternalAudioSource
  - cExternalSink
  - cExternalMessageInterface
  - cVectorBinaryOperation
- New config files:
  - GeMAPS v01b and eGeMAPS v01b (fixing a numeric instability)
- SMILEapi: C API with language bindings for Python and C#.
- iOS platform support and iOS sample app project.
- Colorized log output.
- Command-line option "-exportHelp" to export component help in JSON format for use by 
  third-party applications.
- The growDyn option is now supported for ring-buffer data memory levels.

## Changed
- Replaced Autotools-based build scripts with CMake.
- Rewritten Android sample project using Kotlin and Gradle.
- Major reduction in memory usage for data memory levels with high frame rates 
  (e.g. raw Wave data).
- Performance improvements for components cSpecScale and cFunctionalModulation.
- Improved internal support for components that read data asynchronously from external sources
  (e.g. sound card, network).
- Documentation has been updated and is now provided in HTML format.
- Numerous cleanups and refactorings.

## Fixed
- Numerous bug fixes and other smaller improvements.

## Removed
- Multi-thread processing has been removed for stability reasons.

## [2.3] - 2016-10-28

## Added
- Android JNI integration.  
  While version 2.2. did provide first support for compilation as static binary for Android,
  we now offer integration into Apps via the JNI. An example project and a tutorial for 
  Android Studio are provided which shows how to build a live audio analysis app that
  shows audio parameters in real-time in the App UI.

- Batch feature extraction GUI for Windows and batch extraction scripts for Linux.
  These make it easier for beginners to extract audio features from a collection of audio files.

- Updated Version of ComParE 2013-2015 baseline acoustic parameter set.
  Several optimisations to the feature extraction code were applied and
  an updated version of ComParE was released. It has the same features as 
  the previous version, but is now numerically optimised. It is referred to
  as ComParE_2016, as it is released in 2016. 
  It is not the Interspeech ComParE 2016 baseline feature set (this is still the 2013 one)!!

## Changed
- Configuration file interface updated.
  Commandline options for audio input and data output formats have been standardised
  through modular config files.

## Fixed
- Improved backwards compatibility.
  All standard feature sets are verified to be backwards compatible to Version 2.2 and 2.1.
  The original feature sets of openEAR are also backwards compatible with 
  the original openEAR models.

- Lots of various fixes and improvements.
  We've added many minor points, fixed bugs, and improved performance.
  E.g. we have improved the support for modulation spectra as functionals and optimized 
  Jitter computation through a better pitch period detection algorithm.

## [2.2] - 2015-10-02

## Added
- Configuration files for the first release of the Geneva Minimalistic Acoustic Parameter Set 
  (GeMAPS).

## [2.1] - 2014-12-23

## Added
- Support for reading JSON neural network files created with the CURRENNT toolkit.
- F0 harmonics component.
- Fast linear SVM sink component for integrating models trained with WEKA SMO.
- Various other minor new components and features.

## Fixed
- Improved backwards compatibility of the standard INTERSPEECH challenge parameter sets.
- Various bug fixes.

## [2.0-rc1] - 2013-05

## Added
- Many new components.
- Extended documentation.
- Multi-pass processing mode.
- Support for synchronised audio-visual feature extraction based on OpenCV.

## Changed
- Revised various core components.
- Restructured source tree.

## Fixed
- Many bug fixes.

## [1.0.1] - 2010-09

## Fixed
- Minor bug fixes.

## [1.0.0] - 2010