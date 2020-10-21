.. _about-opensmile:

About openSMILE 
================

We start introducing openSMILE by addressing two important questions for users
who are new to openSMILE: *What is openSMILE?* and *Who needs openSMILE?*. If
you want to start using openSMILE right away, then you should start reading
Section :ref:`get-started`, or Section :ref:`extracting-your-first-features`
if you have already installed openSMILE.


.. _what-is-opensmile:

What is openSMILE?
------------------

The Munich open-Source Media Interpretation by Large feature-space
Extraction (openSMILE) toolkit is a modular and flexible feature
extractor for signal processing and machine learning applications. The
primary focus is clearly put on audio-signal features. However, due to
their high degree of abstraction, openSMILE components can also be used
to analyse signals from other modalities, such as physiological signals,
visual signals, and other physical sensors, given suitable input
components. It is written purely in C++, has a fast, efficient, and
flexible architecture, and runs on common platforms such as
Linux, Windows, MacOS, Android and iOS.

openSMILE is designed for real-time online processing but can also be 
used off-line in batch mode for processing of large data-sets. This is 
a feature rarely found in other tools for feature extraction. Most of 
related projects are designed for off-line extraction and require the 
whole input to be present. openSMILE can extract features incrementally 
as new data arrives. By using the PortAudio [1]_ library, openSMILE 
supports platform-independent live audio input and live audio playback,
enabling the extraction of audio features in real-time.

To facilitate interoperability, openSMILE supports reading and writing
of various data formats commonly used in the field of data mining and
machine learning. These formats include PCM WAVE for audio files, CSV
(Comma Separated Value, spreadsheet format) and ARFF (Weka Data Mining)
for text-based data files, HTK (Hidden-Markov Toolkit) parameter files,
and a simple binary float matrix format for binary feature data.

openSMILE comes with a slim but powerful C API, allowing a seamless
integration into other applications. Wrappers are available for Python,
.NET, Android and iOS. The API provides programmatic access to all 
functionality that is available in the command-line tool, including
passing audio or feature data to and from openSMILE in real-time.

Using the open-source software gnuplot [2]_, extracted features which
are dumped to files can be visualised. A strength of openSMILE, enabled
by its highly modular architecture, is that almost all intermediate data
which is generated during the feature extraction process (such as
windowed audio data, spectra, etc.) can be accessed and saved to files
for visualisation or further processing.


.. _who-needs-opensmile:

Who needs openSMILE?
--------------------

openSMILE is intended to be used primarily for research applications,
demonstrators and prototypes. Thus, the target group of users is
researchers and system developers. Due to its compact code and modular
architecture, using openSMILE in a final product is feasible, as well.
However, we would like to stress that openSMILE is distributed under a
research-only license (see the next section for details).

Currently, openSMILE is used all around the world by researchers and 
companies working in the field of speech recognition (feature
extraction front-end, keyword spotting, etc.), the area of affective
computing (emotion recognition, affect sensitive virtual agents, etc.),
and Music Information Retrieval (chord recognition, beat tracking, onset
detection etc.). Since the 2.0 release, we target the wider
multi-media community by including the popular OpenCV library for video
processing and video feature extraction.


.. _capabilities:

Capabilities
------------

This section gives a brief summary on openSMILE’s capabilities. The
capabilities are distinguished by the following categories: data input,
signal processing, general data processing, low-level audio features,
functionals, classifiers and other components, data output, and other
capabilities.


.. _data-input:

Data input
''''''''''

openSMILE can read data from the following file formats natively:

-  RIFF-WAVE (uncompressed PCM only)

-  Comma Separated Value (CSV)

-  HTK parameter files

-  WEKA’s ARFF format

If built with additional, optional dependencies, openSMILE can also 
read data in the following formats:

-  Any audio format supported by FFmpeg

-  Video streams via OpenCV

In addition, when openSMILE is interfaced via SMILEapi, the following 
two data formats can be passed programmatically as input:

-  Raw float PCM audio samples

-  Raw float feature data

For compatibility, official binaries are not built with FFmpeg 
support and all config files use cWaveSource components by default.
This means that audio stored in compressed formats such as MP3, MP4 
or OGG needs to be converted to uncompressed WAVE format before it 
can be analysed with openSMILE. If you would like to directly process
other audio formats, you need to build openSMILE yourself with FFmpeg 
support and replace uses of cWaveSource with cFFmpegSource in config 
files.

Live recording of audio from any PC sound card is supported via the
PortAudio library. For generating white noise, sinusoidal tones, and
constant values a signal Generator is provided.


.. _signal-processing:

Signal processing
'''''''''''''''''

The following functionality is provided for general signal processing or
signal pre-processing (prior to feature extraction):

-  Windowing-functions (Rectangular, Hamming, Hann (raised cosine),
   Gauss, Sine, Triangular, Bartlett, Bartlett-Hann, Blackmann,
   Blackmann-Harris, Lanczos)

-  Pre-/De-emphasis (i.e. 1st order high/low-pass)

-  Resampling (spectral domain algorithm)

-  FFT (magnitude, phase, complex) and inverse

-  Scaling of spectral axis via spline interpolation

-  dbA weighting of magnitude spectrum

-  Autocorrelation function (ACF) (via IFFT of power spectrum)

-  Average magnitude difference function (AMDF)


.. _data-processing:

Data processing
'''''''''''''''

openSMILE can perform a number of operations for feature normalisation,
modification, and differentiation:

-  Mean-Variance normalisation (off-line and on-line)

-  Range normalisation (off-line and on-line)

-  Delta-Regression coefficients (and simple differential)

-  Weighted Differential as in :cite:`Schuller2007far`

-  Various vector operations: length, element-wise addition,
   multiplication, logarithm and power

-  Moving average filter for smoothing of contour over time


.. _audio-features-low-level:

Audio features (low-level)
''''''''''''''''''''''''''

The following (audio-specific) low-level descriptors can be computed by
openSMILE:

-  Frame Energy

-  Frame Intensity / Loudness (approximation)

-  Critical Band spectra (Mel/Bark/Octave, triangular masking filters)

-  Mel-/Bark-Frequency-Cepstral Coefficients (MFCC)

-  Auditory Spectra

-  Loudness approximated from auditory spectra

-  Perceptual Linear Predictive (PLP) Coefficients

-  Perceptual Linear Predictive Cepstral Coefficients (PLP-CC)

-  Linear Predictive Coefficients (LPC)

-  Line Spectral Pairs (LSP, aka. LSF)

-  Fundamental Frequency (via ACF/Cepstrum method and via
   Subharmonic-Summation (SHS))

-  Probability of Voicing from ACF and SHS spectrum peak

-  Voice-Quality: Jitter and Shimmer

-  Formant frequencies and bandwidths

-  Zero and Mean Crossing rate

-  Spectral features (arbitrary band energies, roll-off points,
   centroid, entropy, maxpos, minpos, variance (= spread), skewness,
   kurtosis, slope)

-  Psychoacoustic sharpness, spectral harmonicity

-  CHROMA (octave-warped semitone spectra) and CENS features
   (energy-normalised and smoothed CHROMA)

-  CHROMA-derived features for Chord and Key recognition

-  F0 Harmonics ratios


.. _video-features-low-level:

Video features (low-level)
''''''''''''''''''''''''''

The following video low-level descriptors can be currently computed by
openSMILE, based on the OpenCV library:

-  HSV colour histograms

-  Local binary patterns (LBP)

-  LBP histograms

-  Optical flow and optical flow histograms

-  Face detection: all these features can be extracted from an
   automatically detected facial region or from the full image.


.. _functionals:

Functionals
'''''''''''

In order to map contours of audio and video low-level descriptors onto a
vector of fixed dimensionality, the following functionals can be
applied:

-  Extreme values and positions

-  Means (arithmetic, quadratic, geometric)

-  Moments (standard deviation, variance, kurtosis, skewness)

-  Percentiles and percentile ranges

-  Regression (linear and quadratic approximation, regression error)

-  Centroid

-  Peaks

-  Segments

-  Sample values

-  Times/durations

-  Onsets/Offsets

-  Discrete Cosine Transformation (DCT)

-  Zero Crossings

-  Linear Predictive Coding (LPC) coefficients and gain


.. _classifiers-and-other-components:

Classifiers and other components
''''''''''''''''''''''''''''''''

Live demonstrators for audio processing tasks often require segmentation
of the audio stream. openSMILE provides voice activity detection
algorithms and a turn detector for this purpose. For incremental
classification of extracted features, Support Vector Machines are
implemented using the LIBSVM library.

-  Voice Activity Detection based on Fuzzy Logic

-  Voice Activity Detection based on LSTM-RNN with pre-trained models

-  Turn/Speech segment detector

-  LIBSVM (on-line)

-  SVM sink (for loading WEKA SMO models using a linear kernel)

-  GMM (experimental implementation from the eNTERFACE’12 project, to be
   released soon)

-  LSTM-RNN (Neural Network) classifier which can load RNNLIB and
   CURRENNT nets

-  Speech Emotion recognition pre-trained models (openEAR)


.. _data-output:

Data output
'''''''''''

For writing data to files, the following formats are natively supported:

-  RIFF-WAVE (PCM uncompressed audio)

-  Comma Separated Value (CSV)

-  HTK parameter file

-  WEKA ARFF file

-  LIBSVM feature file format

-  Binary float matrix format

In addition, when openSMILE is interfaced via SMILEapi, the following 
types of data can be received programmatically as output:

-  Raw float PCM audio samples

-  Raw float feature data

-  Component messages

Additionally, live audio playback is supported via the PortAudio
library.


.. _other-capabilities:

Other capabilites
'''''''''''''''''

Besides input, signal processing, feature extraction, functionals and
output components, openSMILE comes with a few other capabilites (to
avoid confusion, we do not use the term ‘features’ here):

Multi-threading
    Independent components can be run in parallel to make use of
    multiple CPUs or CPU cores and thus speed up feature extraction
    where performance is critical.

    .. note::
        Multi-threaded processing is only supported in openSMILE 
        versions 2.3 and lower. Starting with version 3.0, the setting 
        is ignored and processing always uses a single thread.

Plugin support
    Additional components can be built as shared libraries (DLLs on
    Windows) linked against openSMILE’s core API library. Such plugins
    are automatically detected during initialisation of the program if
    they are placed in the ``plugins`` subdirectory.

Extensive logging
    Log messages can be printed to the standard error output, saved to 
    a file or received programmatically via SMILEapi. The detail of the
    messages can be controlled via a log level setting. For easier 
    interpretation of the messages, the types *Message* (MSG), 
    *Warning* (WRN), *Error* (ERR), and *Debug* (DBG) are distinguished.

Flexible configuration
    openSMILE can be fully configured via one single text-based
    configuration file. This file is kept in a simple, yet very
    powerful, INI-style file format. Each component has its own section
    and all components can be connected to a central data memory
    component. The configuration file even allows for defining custom
    command-line options (e.g. for input and output files), and
    inclusion of other configuration files to build reusable modular
    configuration blocks. The name of the configuration file to include
    can be specified on the command-line, allowing maximum flexibility
    in scripting.

Incremental processing
    All components in openSMILE follow strict guidelines to meet the
    requirements of incremental processing. It is not allowed to require
    access to the full input sequence and seek back and forth within the
    sequence, for example. Principally each component must be able to
    process its data frame by frame or at least as soon as possible.
    Some exceptions to this rule have been granted for components which
    are only used during off-line feature extraction, such as components
    for full-input mean normalisation.

Multi-pass processing in batch mode
    For certain tasks, multi-pass processing is required, which
    obviously can only be applied in off-line (or buffered) mode.
    openSMILE 2.0 and higher supports multi-pass processing for all
    existing components.

.. [1]
   http://www.portaudio.com

.. [2]
   http://www.gnuplot.info/


.. _history-and-changelog:

History and Changelog
---------------------

openSMILE was originally created in the scope of the European EU-FP7
research project SEMAINE (http://www.semaine-project.eu) and is used
there as the acoustic emotion recognition engine and keyword spotter in
a real-time affective dialogue system. To serve the research community,
open-source releases of openSMILE were made independently of the main
project’s code releases.

The first publicly available version of openSMILE was contained in the
first Emotion and Affect recognition toolkit openEAR as the feature
extraction core. openEAR was introduced at the Affective Computing and
Intelligent Interaction (ACII) conference in 2009. One year later, the
first independent release of openSMILE (version 1.0.0) was published,
which aimed at reaching a wider community of audio analysis researchers.
It was presented at ACM Multimedia 2010 in the Open-Source Software
Challenge. This first release was followed by a small bugfix release
(1.0.1) shortly afterwards. Since then, development has taken place in
the subversion repository on SourceForge. Since 2011, the development
has been continued in a private repository due to various licensing
issues with internal and third-party projects.

openSMILE 2.0 (rc1) is the next major release after the 1.0.1 version
and contains revised core components and a long list of bugfixes, new
components, as well as improved old components, extended documentation,
a restructured source tree and new major functionality such as a
multi-pass mode and support for synchronised audio-visual feature
extraction based on OpenCV.

Version 2.1 contains further fixes, improved backwards compatibility of
the standard INTERSPEECH challenge parameter sets, support for reading
JSON neural network files created with the CURRENNT toolkit, an F0
harmonics component, a fast linear SVM sink component for integrating
models trained with WEKA SMO, as well as a number of other minor new
components and features. It is the first version published and supported
by audEERING.

Version 2.2 comes with the configuration files for the first release of
the Geneva Minimalistic Acoustic Parameter Set (GeMAPS).

Version 2.3 includes Android JNI integration, an updated configuration
file interface, a batch feature extraction GUI for Windows, improved
backwards compatibility, an updated version of the ComParE 2013–2015
baseline acoustic parameter set, as well as several bugfixes and
performance improvements.

Version 3.0 is the third major release of openSMILE featuring a large 
number of incremental improvements and fixes. Most notably, it 
introduces the new SMILEapi C API and a standalone Python library. 
Other changes include a fully rewritten build process using CMake, 
support for the iOS platform, an updated Android integration, 
an FFmpeg audio source component, major performance and memory usage 
improvements, documentation in HTML format, and numerous other minor 
updates, code refactorings and fixes. Beginning with version 3.0, 
openSMILE binaries and source code are hosted on GitHub.


.. _licensing:

Licensing
---------

openSMILE follows a dual-licensing model. Since the main goal of the
project is a widespread use of the software to facilitate research in
the field of machine learning from audio-visual signals, the source code
and binaries are freely available for private, research, and educational
use under an open-source license. It is not allowed to use the
open-source version of openSMILE for any sort of commercial product.
Fundamental research in companies, for example, is permitted, but if a
product is the result of the research, we require you to buy a
commercial development license. Contact us at info@audeering.com (or
visit us at https://www.audeering.com) for more information.