.. _developer-s-documentation:

Developer’s documentation
=========================

.. _writing-components:

Writing components
''''''''''''''''''

Getting started
~~~~~~~~~~~~~~~

In the ``src`` directory, some examples for various component types are 
provided which you may use as a starting point. These files contain some 
helpful documentation as comments in the source.

In order to create a new component cMySmileComponent, you typically need to 
make at least the following changes:

* provide a main component implementation file in an appropriate subfolder of 
  ``src``, e.g. ``src/dsp/mySmileComponent.cpp``

* put the corresponding header file in an appropriate sub-folder of 
  ``src/include``, e.g. ``src/include/dsp/mySmileComponent.hpp``

* in ``src/include/core/componentList.hpp``:

  * include your header file at the top, e.g. 
    ``#include <dsp/mySmileComponent.hpp>``

  * include the ``registerComponent`` method of your class in the 
    ``componentlist`` variable, e.g. ``cMySmileComponent::registerComponent,``

* add the path of your component cpp file to the ``opensmile_SOURCES`` 
  variable in CMakeLists.txt, e.g. ``src/dsp/mySmileComponent.cpp``

* if your component has special external build dependencies, you may need to 
  make corresponding additions to CMakeLists.txt

You may use the Perl script ``clonecomp.pl`` to copy the definition of an 
existing component:

::

    perl clonecomp.pl <inputCompBase> <yourCompBase> <inputCompName> <yourCompName> 

This will create corresponding cpp and hpp files.

For a complete list of openSMILE components, run ``SMILExtract -L``.
Each component is a descendant of cSmileComponent. This base class encapsulates
all the functionality required for accessing the configuration data, managing 
the configuration and finalisation process of all components, and running 
components ('ticking').

Basic component types
~~~~~~~~~~~~~~~~~~~~~

Generally speaking, there are three basic types of components used in openSMILE:

#. **Data Sources** (cDataSource descendants, see ``exampleSource.cpp``)
   
   A data source contains a writer sub-component (cDataWriter), which is 
   responsible for writing data to exactly one level of the data memory 
   (see cDataProcessor description below).

   Implement a descendant of this component if you want to add a new input 
   format, e.g. MP3 or feature file import (HTK, ARFF, etc.).

#. **Data Processors** (cDataProcessor descendants, see ``exampleProcessor.cpp``)

   A data processor contains both reader and writer components (cDataReader 
   and cDataWriter). The general purpose of a data processor is to read data 
   from the data memory (from one or more levels) and write data to one single
   level (NOTE: each writer has exclusive access to exactly one level, i.e. 
   each level is written to by exactly one writer and thus by exactly one data
   processor or data source component).

   This component is the one you most likely want to inherit if you want to 
   implement new features. Please also see below, for special kinds of data 
   processors for common processing tasks!

#. **Data Sinks** (cDataSink descendants, see ``exampleSink.cpp``)

   A data sinks contains a reader sub-component (cDataReader), which is 
   responsible for the 'read' interface to a specific data memory level (or 
   multiple levels).

   Implement a descendant of this component if you want to add support for 
   more data output formats (e.g. sending data over a network, real-time 
   visualisation of data via a GUI, etc.).

Since Data Processors are very general components, three special descendants 
have been implemented:

* **cVectorProcessor**

  This class allows an easy frame by frame processing of data (mostly 
  processing of feature frames in the spectral domain). Input framesize can be
  different from the output framesize, thus it is very flexible. Algorithms 
  such as signal window function, FFT, Mfcc, Chroma, etc. are implemented 
  using cVectorProcessor as base. See ``exampleVectorProcessor.cpp`` as an 
  example to start with.

* **cWindowProcessor**

  This class allows processing of signal windows (e.g. filters, functionals, 
  etc.). The main functionality provided is automatic overlapping of signal 
  windows, i.e. for having access to past and future samples in a certain 
  window, yet offering the possibility of block processing for efficient 
  algorithms. See ``preemphasis.cpp`` for an example.

* **cWinToVecProcessor**

  This class takes data from a signal window and produces a single value or 
  vector of values (frame) for this window. You can specify an overlap (via 
  frameStep and frameSize). This class is used for windowing the input 
  wave-form signal, but can also be inherited for implementing data summaries
  (i.e. statistical functionals). See framer.cpp/hpp for an example 
  implementation of a descendant class.

.. _writing-plugins:

Writing plugins
'''''''''''''''

openSMILE allows to be extended using plugins that add additional
components at runtime. Adjust the CMake build script in the
``plugindev`` directory for building your plugin.

The main source file of a plugin is the ``plugindev/pluginMain.cpp``
file. This file includes the individual component files, similar to the
component list in the ``componentManager.cpp`` file which manages the
openSMILE built-in components.
