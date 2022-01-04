.. _reference-section:

Reference section
=================

This section (:ref:`smilextract-command-line-options`) documents available command-line
options and describes the general usage of the ``SMILExtract`` command-line
tool. A documentation of the configuration file format can be found in
Section :ref:`configuration-files`. 
If you are interested what is going on inside openSMILE, which components exist
besides those that are instantiable and connectable via the configuration
files, and to learn more about the terminology used, then you should read
Section :ref:`opensmile-architecture` which describes the program
architecture in detail.
Section :ref:`components` consists of a comprehensive list of all components 
included with openSMILE and provides detailed documentation for each component.
Finally, Section :ref:`smileapi-c-api-and-wrappers` covers the SMILEapi which 
allows to seamlessly integrate openSMILE into other applications.


.. _smilextract-command-line-options:

SMILExtract command-line options
--------------------------------

The ``SMILExtract`` binary is a very powerful command-line utility which
includes all the built-in openSMILE components. Using a single ini-style
configuration file, various modes of operation can be configured. This
section describes the command-line options available when calling
``SMILExtract``. Some options take an optional parameter, denoted by
``[parameter-type]``, while some require a mandatory
parameter, denoted by ``<parameter-type>``.

| **Usage:** SMILExtract [-option (value)] ...
|
| **-h**
|                          Show this usage information
|
| **-C, -configfile**      <*string*>
|                          Path to openSMILE config file.
|                          *Default:* ’smile.conf’
|
| **-l, -loglevel**        <*int*>
|                          Verbosity level of log messages (MSG, WRN, ERR, DBG)
                           (0-9). 1: only important messages, 2,3: more
                           detailed messages, 4,5: very detailed debug messages
                           (if -debug is enabled), 6+: currently unused.
|                          *Default:* 2
|
| **-t, -nticks**          <*int*>
|                          Number of ticks (component loop iterations) to
                           process (-1 = infinite) (Note: this only works for
                           single thread processing, i.e. nThreads=1 set in the
                           config file). This option is not intended for normal
                           use. It is for debugging component execution code
                           only.
|                          *Default:* -1
|
| **-d, -debug**
|                          Show debug log-messages (DBG) (this is only available
                           if the binary was compiled as a debug build)
|                          *Default:* off
|
| **-L, -components**
|                          Show full component list (this list includes
                           plugins, if they are detected correctly), and exit.
|
| **-H, -configHelp**      [*componentName:string*]
|                          Show the documentation of configuration options of
                           all available components (including plugins) and
                           exit. If the optional string parameter is given, then
                           only documentation of components beginning with the
                           given string will be shown.
|
| **-configDflt**          [*string*]
|                          Show default configuration file section templates for
                           all components available (empty parameter), or
                           selected components beginning with the string given
                           as parameter. In conjunction with the
                           ’cfgFileTemplate’ option a comma separated list of
                           components can be passed as parameter to
                           ’configDflt’, to generate a template configuration
                           file with the listed components.
|
| **-cfgFileTemplate**
|                          Experimental functionality to print a configuration
                           file template containing the components specified in
                           a comma separated string as argument to the
                           ’configDflt’ option.
|
| **-cfgFileDescriptions**
|                          If this option is set, then option descriptions will
                           be included in the generated template configuration
                           files.
|                          *Default:* off
|
| **-c, -ccmdHelp**
|                          Show user defined command-line option help in
                           addition to the standard usage information (as
                           printed by ’-h’). Since openSMILE provides means to
                           define additional command-line options in the
                           configuration file, which are available only after
                           parsing the configuration file, and additional
                           command-line option has been introduced to show a
                           help on these options. A typical command-line to
                           show this help would be
                           ``SMILExtract -c -C myconfigfile.conf``.
|
| **-exportHelp**
|                          Print detailed documentation of registered config 
                           types in JSON format. This option is intended for
                           external tools that want to consume the on-line help
                           content programmatically.
|                          *Default:* off
|
| **-logfile**             <*string*>
|                          Specifies the path and filename of a log file.
                           Make sure the path of the file is writeable.
|                          *Default:* off
|
| **-appendLogfile**
|                          If this option is specified, openSMILE will append
                           log messages to an existing log-file instead of
                           overwriting the log-file at every program start
                           (which is the default behaviour).
|
| **-noconsoleoutput**
|                          If this option is specified, no log-output is
                           displayed in the console. Logging to the log file,
                           if enabled, is not affected by this option.


.. _opensmile-architecture:

openSMILE architecture
----------------------

The SMILExtract binary is the main application which can run all
configuration files. If you take a look at the source code of it (which
is found in ``opensmile/progsrc/smilextract/SMILExtract.cpp``), you will see 
that it is fairly short.
It uses the classes from the openSMILE API to create the components and
run the configurations. These API functions can be used in custom applications,
such as GUI front-ends etc. Therefore, they will be described in more detail in
the developer’s documentation in Section :ref:`developer-s-documentation`.
However, to obtain a general understanding what components make openSMILE run,
how they interact, and in what phases the program execution is split, a brief
overview is given in this section.

openSMILE’s application flow can be split into three general phases:

Pre-config phase
    Command-line options are read and the configuration file is parsed.
    Also, usage information is displayed, if requested, and a list of
    built-in components is generated.

Configuration phase
    The component manager is created and instantiates all components
    listed in its ``instances`` configuration array. The configuration
    process is then split into 3 phases, where components first register
    with the component manager and the data memory, then perform the
    main configuration steps such as opening of input/output files,
    allocation of memory, etc., and finally finalise their configuration
    (e.g. set the names and dimensions of their output fields, etc.).
    Each of the 3 phases is passed through several times, since some
    components may depend on other components having finished their
    configuration (e.g. components that read the output from another
    component and need to know the dimensionality of the output and the
    names of the fields in the output). Errors, due to
    mis-configurations, bogus input values, or inaccessible files, are
    likely to happen during this phase.

Execution phase
    When all components have been initialised successfully, the
    component manager starts the main execution loop (also referred to
    as tick-loop). Every component has a tick() method, which implements
    the main incremental processing functionality and reports on the
    status of the processing via its return value.

    In one iteration of the execution loop, the component manager calls
    all tick() functions in series (*Note:* the behaviour is different,
    when components are run in multiple threads). The loop is continued
    as long as at least one component’s tick() method returns a non-zero
    value (which indicates that data was processed by this component).

    If all components indicate that they did not process data, it can be
    safely assumed that no more data will arrive and the end of the
    input has been reached (this may be slightly different for on-line
    settings, however, it is up to the source components to return a
    positive return value or pause the execution loop, while they are
    waiting for data).

    When the end of the input is reached, the component manager signals
    the end-of-input condition to the components by running one final
    iteration of the execution loop. After that, the execution loop will
    be ran anew, until all components report a failure status. This
    second phase is referred to end-of-input processing. It is mainly
    used for off-line processing, e.g. to compute features from the last
    (but incomplete) frames, to mean normalise a complete sequence, or
    to compute functionals from a complete sequence.

openSMILE contains three classes which cannot be instantiated from the
configuration files. These are the command-line parser
(ccommand-lineParser), the configuration manager (cConfigManager), and
the component manager (cComponentManager). We will now briefly describe
the role of each of these in a short paragraph. The order of the
paragraph corresponds to the order the classes are created during
execution of the SMILExtract program.


.. _the-command-line-parser:

The command-line parser
~~~~~~~~~~~~~~~~~~~~~~~

This class parses the command-line and provides options in an easily
accessible format to the calling application. Simple command-line syntax
checks are also performed. After the configuration manager has been
initialised and the configuration has been parsed, the command-line is
parsed a second time, to also get the user-defined command-line options
set in the current configuration file.


.. _the-configuration-manager:

The configuration manager
~~~~~~~~~~~~~~~~~~~~~~~~~

The configuration manager loads the configuration file, which was
specified on the SMILExtract command-line. Thereby, configuration
sections are split and then parsed individually. The configuration
sections are stored in an abstract representation as ConfigInstance
classes (the structure of these classes is described by a ConfigType
class). Thus, it is easy to add additional parsers for formats other
than the currently implemented ini-style format.


.. _the-component-manager:

The component manager
~~~~~~~~~~~~~~~~~~~~~

The component manager is responsible for instantiating, configuring, and
executing the components. The details have already been described in the
above section on openSMILE’s application flow. Moreover, the component
manger is responsible for enumerating and registering components in
plugins. Therefore, a directory called ``plugins`` is scanned for binary
plugins. The plugins found are registered, and become useable exactly in
the same way as built-in components. A single plugin binary thereby can
contain multiple openSMILE components.

The components instantiated by the component manager are all descendants
of the cSmileComponent class. They have two basic means of standardised
communication: a) directly and asynchronously, via smile messages, and
b) indirectly and synchronously via the data memory.

Method a) is used to send out-of-line data, such as events and
configuration changes directly from one smile component to another.
Classifier components, for example, send a ‘classificationResult’
message which can be caught by other components (esp. custom plug-ins),
to change their behaviour or send the message to external sources.

Method b) is the standard method for handling of data in openSMILE. The
basic principle is that of a data source producing a frame of data and
writing it to the data memory. A data processor reads this frame,
applies some fancy algorithm to it, and writes a modified output frame
back to a different location in the data memory. This step can be
repeated for multiple data processors. Finally, a data sink reads the
frame and passes it to an external source or interprets (classifies) it
in some way. The advantage of passing data indirectly is that multiple
components can read the same data, and data from past frames can stored
efficiently in a central location for later use.


.. _incremental-processing:

Incremental processing
~~~~~~~~~~~~~~~~~~~~~~

.. _fig-arch:

.. figure:: _static/images/arch.*
    :alt: Overview on openSMILE's component types and openSMILE's basic
        architecture.
    :width: 70 %

    Overview on openSMILE's component types and openSMILE's basic
    architecture.

The data-flow in openSMILE is handled by the cDataMemory component. This
component manages multiple data memory ‘levels’ internally. These levels
are independent data storage locations, which can be written to by
exactly one component and read by an arbitrary number of components.
From the outside (the component side) the levels appear to be a
:math:`N x \infty` matrix, with :math:`N` rows, whereby :math:`N` is the
frame size. Components can read/write frames (columns) at/to any
location in this virtual matrix. Data memory levels can be stored 
internally either as ring-buffers (isRb=1) or regular buffers (isRb=0) 
and their size can be either fixed (growDyn=0) or growing dynamically at
runtime (growDyn=1). For fixed-size ring buffers, a write operation only 
succeeds if there are empty frames in the buffer (frames that have not been
written to yet, or frames that have been read by all components reading from
the level), and a read operation only succeeds if the referred frame index 
lies no more than the ring buffer size in the past. For fixed-size regular 
buffers, writes will succeed until the buffer is full, after that writes will
always fail. For dynamically growing levels, writes always succeed, except
when the application is out-of-memory. Be aware that there is no limitation
on the amount of memory allocated for dynamically growing levels. In almost
all cases, a fixed-size ring buffer is the recommended level configuration.

:numref:`fig-arch` shows the overall data-flow architecture of openSMILE,
where the data memory is the central link between all dataSource,
dataProcessor, and dataSink components.

.. _fig-inc1:

.. figure:: _static/images/incremental-processing/figure1.*
    :alt: Partially filled buffers.
    :width: 70 %

    Incremental processing with ring-buffers. Partially filled buffers.

.. _fig-inc2:

.. figure:: _static/images/incremental-processing/figure2.*
    :alt: Filled buffers with warped read/write pointers.
    :width: 70 %

    Incremental processing with ring-buffers. Filled buffers with warped
    read/write pointers.

The ring-buffer based incremental processing is illustrated in
:numref:`fig-inc1` and :numref:`fig-inc2`. Three levels are present in this
setup: wave, frames, and pitch. A cWaveSource component writes samples to the
‘wave’ level. The write positions in the levels are indicated by a red arrow. A
cFramer produces frames of size 3 from the wave samples (non-overlapping), and
writes these frames to the ‘frames’ level. A cPitch (a component with this name
does not exist, it has been chosen here only for illustration purposes)
component extracts pitch features from the frames and writes them to the ‘pitch’
level. In :numref:`fig-inc2` the buffers have been filled, and the write
pointers have been warped. Data that lies more than ‘buffersize’ frames in the
past has been overwritten.

.. _fig-inc3:

.. figure:: _static/images/incremental-processing/figure3.*
    :alt: Incremental computation of high-level features such as
        statistical functionals.
    :width: 70 %

    Incremental computation of high-level features such as statistical
    functionals.

:numref:`fig-inc3` shows the incremental processing of higher order
features. Functionals (max and min) over two frames (overlapping) of the
pitch features are extracted and saved to the level ‘func’.

The size of the buffers must be set correctly to ensure smooth
processing for all blocksizes. A ‘blocksize’ thereby is the size of the
block a reader or writer reads/writes from/to the dataMemory at once. In
the above example, the read blocksize of the functionals component would
be 2 because it reads two pitch frames at once. The input level buffer
of ‘pitch’ must be at least 2 frames long, otherwise the functionals
component will never be able to read a complete window from this level.

openSMILE handles automatic adjustment of the buffersizes. Therefore,
readers and writers must register with the data memory during the
configuration phase and publish their read and write blocksizes. The
minimal buffersize is computed based on these values. If the buffersize
of a level is set smaller than the minimal size, the size will be
increased to the minimum possible size. If the specified size (via
configuration options) is larger than the minimal size, the larger size
will be used.

.. note::

    This automatic buffersize setting only applies to ring-buffers. If 
    you use non-ring buffers, or if you want to process the full input 
    (e.g. for functionals of the complete input, or mean normalisation)
    it is always recommended to configure a dynamically growing 
    non-ring buffer level (see cDataWriter configuration in the 
    :ref:`configuring-components` section for details).


.. _opensmile-terminology:

openSMILE terminology
~~~~~~~~~~~~~~~~~~~~~

In the context of the openSMILE data memory, various terms are used which
require clarification and a precise definition, such as ‘field’,
‘element’, ‘frame’, and ‘window’.

You have learnt about the internal structure of the dataMemory in
Section :ref:`incremental-processing`. Thereby a level in the data memory
represents a unit which contains numeric data, frame meta data, and temporal
meta data. Temporal meta data is present on the one hand for each frame, thereby
describing frame timestamps and custom per frame meta information. On the
other hand, temporal meta data is present globally, describing the global 
frame period and timing mode of the level.

If we view the numeric contents of the data memory level as a 2D
:math:`<`\ nFields x nTimestemps\ :math:`>` matrix, where ‘frames’ correspond
to the columns of this matrix, and ‘windows’ or ‘contours’ correspond
the rows of this matrix. The frames are also referred to as
(column-)‘vectors’ in some places. (*Note:* when exporting data to
files, the data, viewed as matrix, is transposed, i.e. for text-based
files (CSV, ARFF), the rows of the file correspond to the frames.) The
term ‘elements’, as used in openSMILE, refers to the actual elements of
the frames/vectors. The term ‘field’ refers to a group of elements that
belong together logically and where all elements have the same name.
This principle shall be illustrated by an example: A feature frame
containing the features ‘energy’, ‘F0’, and 'MFCC' 1-6, will have
:math:`1+1+6=8` elements, but only :math:`3` fields: the field ‘energy’
with a single element, the field ‘F0’ with a single element, and the
(array-) field ‘MFCC’ with 6 elements (called ‘MFCC[0]’ – ‘MFCC[1]’).


.. _configuration-files:

Configuration files
-------------------

openSMILE configuration files follow an INI-style file format. The file
is divided into sections, which are introduced by a section header:

::

    [sectionName:sectionType]

The section header, as opposed to the standard INI format, always contains two
parts, the section name (first part) and the section type (second part). The two
parts of the section header are separated by a colon (:). The section body (the
part after the header line up to the next header line or the end of the file)
contains attributes (which are defined by the section type; a description of the
available types can be seen using the ``-H`` command-line option as well as in
Section :ref:`components`). Attributes are given as
``name = value`` pairs. An example of a generic configuration file section is
given here:

::

    [instancename:configType]     <-- this specifies the header 
    variable1 = value             <-- example of a string variable
    variable2 = 7.8               <-- example of a "numeric" variable
    variable3 = X                 <-- example of a "char" variable
    subconf.var1 = myname         <-- example of a variable in a sub type
    myarr[0] = value0             <-- example of an array 
    myarr[1] = value1
    anotherarr = value0;value1    <-- example of an implicit array
    noarray = value0\;value1      <-- use \; to quote the separator ';'
    strArr[name1] = value1        <-- associative arrays, name=value pairs
    strArr[name2] = value2

    ; line-comments may be expressed by ; // or # at the beginning
    /* multi-line comments (C style):
       NOTE: comments must start at the beginning of a line  and must end 
       at the end of a line. Comments within a line are not supported. */
    variable4 = value // end-of-line comments

In principal, config type names can be arbitrary strings. However, for
consistency, the names of the components and their corresponding
configuration type names are identical. Thus, to configure a component
``cWaveSource`` you need a configuration section of type
``cWaveSource``.

In every openSMILE configuration file, there is one mandatory section
which configures the component manager. This is the component that
instantiates and runs all other components. The following sub-section
describes this section in detail.


.. _enabling-components:

Enabling components
~~~~~~~~~~~~~~~~~~~

The components which will be run, can be specified by configuring the
``cComponentManager`` component, as shown in the following listing (the
section must be called ``componentInstances``):

::

    [componentInstances:cComponentManager]     <-- don't change this
    ; one data memory component must always be specified!
    ; the default name is 'dataMemory'
    ; if you call your data memory instance 'dataMemory',
    ; you will not have to specify the reader.dmInstance variables 
    ; for all other components!
    ; NOTE: you may specify more than one data memory component
    ; configure the default data memory:
    instance[dataMemory].type = cDataMemory      
    ; configure a sample data source (name = source1):
    instance[source1].type = cExampleSource

The associative array ``instance`` is used to configure the list of
components. The component instance names are specified as the array keys
and are freely definable. They can contain all characters except for ],
however, it is recommended to only use alphanumeric characters, \_, and
-. The component types (i.e. which component to instantiate), are given
as value to the ``type`` option.

.. note::

    For each component instance specified in the ``instance`` array, a 
    configuration section *must* exist in the file (*except for the 
    data memory components!*), even if it is empty (e.g. if you want to
    use default values only). In this case, you need to specify only 
    the header line ``[name:type]``.


.. _configuring-components:

Configuring components
~~~~~~~~~~~~~~~~~~~~~~

The parameters of each component can be set in the configuration section
corresponding to the specific component. For a wave source, for example,
(which you instantiate with the line

::

    instance[source1].type = cWaveSource

in the component manager configuration) you would add the following
section (note that the name of the configuration section must match the
name of the component instance, and the name of the configuration type
must match the component’s type name):

::

    [source1:cWaveSource]
     ; the following sets the level this component writes to
     ; the level will be created by this component
     ; no other components may write to a level having the same name
    writer.dmLevel = wave
    filename = input.wav

This sets the file name of the wave source to ``input.wav``. Further, it
specifies that this wave source component should write to a data memory
level called ``wave``. Each openSMILE component, which processes data
has at least a data reader (of type cDataReader), a data writer (of type
cDataWriter), or both. These sub-components handle the interface to the
data memory component(s). The most important option, which is mandatory,
is ``dmLevel``, which specifies the level to write to or to read from.
Writing is only possible to one level and only one component may write
to each level. We would like to note at this point that the levels do
not have to be specified implicitly by configuring the data memory—in
fact, the data memory is the only component which does not have and does
not require a section in the configuration file—rather, the levels are
created implicitly through ``writer.dmLevel = newlevel``. Reading is
possible from more than one level. Thereby, the input data will be
concatenated frame-wise to one single frame containing data from all
input levels. To specify reading from multiple levels, separate the
level names with the array separator ’;’, e.g.:

::

    reader.dmLevel = level1;level2

The next example shows the configuration of a ``cFramer`` component
``frame``, which creates (overlapping) frames from raw wave input, as
read by the wave source:

::

    [frame:cFramer]
    reader.dmLevel = wave
    writer.dmLevel = frames
    frameSize = 0.0250
    frameStep = 0.010

The component reads from the level ``wave``, and writes to the level
``frames``. It will create frames of 25ms length at a rate of 10ms. The
actual frame length in samples depends on the sampling rate, which will
be read from meta-information contained in the ``wave`` level. For more
examples please see Section :ref:`default-feature-sets`.


.. _including-other-configuration-files:

Including other configuration files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To include other configuration files into the main configuration file
use the following command on a separate line at the location where you
want to include the other file:

::

    \{path/to/config.file.to.include}

This include command can be used anywhere in the configuration file (as
long it is on a separate line). It simply copies the lines of the
included file into the main file while loading the configuration file
into openSMILE .


.. _linking-to-command-line-options:

Linking to command-line options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

openSMILE allows for defining of new command-line options for the
``SMILExtract`` binary in the configuration file. To do so, use the
``\cm`` command as value, which has the following syntax:

::

    \cm[longoption(shortoption){default value}:description text]

The command may be used as illustrated in the following example:

::

    [exampleSection:exampleType]
    myAttrib1 = \cm[longoption(shortopt){default}:descr. text]
    myAttrib2 = \cm[longoption{default}:descr. text]

The ``shortopt`` argument and the ``default value`` are optional. Note
that, however, either ``default`` and/or ``descr. text`` are required to
define a *new* option. If neither of the two is specified, the option
will not be added to the command-line parser. You can use this mode to
reference options that were already added, i.e. if you want to use the
value of an already existing option which has been defined at a prior
location in the config file:

::

    [exampleSection2:exampleType]
    myAttrib2 = \cm[longoption]

An example for making a filename configurable via the command-line, is
given here:

::

    filename = \cm[filename(F){default.file}:use this option to specify the filename for the XYZ component]

You can call ``SMILExtract -c -C yourconfigfile.conf`` to see your
command-line options appended to the general usage output.

.. note::

    When specifying command-line options as a value to an option, the 
    ``\cm`` command is the only text allowed at the right side of the 
    equal sign! Something like ``key = value \cm[...]`` is currently
    not allowed. The \\cm command may also only appear in the value 
    field of an assignment and (since version 2.0) also instead of a 
    filename in the config file include command.


.. _defining-variables:

Defining variables
~~~~~~~~~~~~~~~~~~

This feature is not yet supported, but is planned for addition. This
should help avoid duplicate values and increase maintainability of
configuration files. A current workaround is to define a command-line
option with a given default value instead of a variable.


.. _comments:

Comments
~~~~~~~~

Single line comments may be initiated by the following characters at the
beginning of the line (only whitespaces may follow the characters):
``; # // %``

If you want to comment out a partial line, please use ``//``. Everything
following the double slash on this line (and the double slash itself)
will be considered a comment and will be ignored.

Multi-line comments are now supported via the C-style sequences ``/*``
and ``*/``. In order to avoid parser problems here, please make sure
these sequences are on a separate line, e.g.

::

    /*
    [exampleSection2:exampleType]
    myAttrib2 = \cm[longoption]
    */

and not:

::

    /*[exampleSection2:exampleType]
    myAttrib2 = \cm[longoption]*/

The latter case is supported, however, you must ensure that the closing
``*/`` is *not* followed by *any* whitespaces.


.. _ide-support-for-configuration-files:

IDE support for configuration files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A community-provided extension for Visual Studio Code is available at
https://marketplace.visualstudio.com/items?itemName=chausner.opensmile-config-files.

The extension integrates the on-line help system of openSMILE into the
IDE and also provides syntax highlighting, statement completion,
diagnostics and code navigation features for config files.


.. _components:

Components
----------

This section contains a list of all components in openSMILE and available options
for each component. You may also access the same information via openSMILE's inbuilt
help system. From the command-line you can get a list of available components 
(and a short description for each) with the command

::

    SMILExtract -L

All available configuration options for a specific component (replace
cMyComponentName by the actual name), as well as the description of
their use and meaning, can be obtained with the command

::

    SMILExtract -H cMyComponentName

.. include:: components.txt


.. _smileapi-c-api-and-wrappers:

SMILEapi C API and wrappers
---------------------------

The command-line SMILExtract application is handy to perform feature
extraction on a set of files for research purposes or to run openSMILE 
as part of a demonstrator. A seamless integration of openSMILE 
functionality into applications is not possible with this approach,
however. For these usecases, openSMILE provides a C API called SMILEapi 
which can be called directly from other applications. All functionality
available in the SMILExtract command-line tool is also available 
programmatically via SMILEapi. In addition, it allows to pass and
retrieve audio, features or smile messages to and from openSMILE in
real-time.

The SMILEapi library is built as part of the normal build process,
the binaries are created under ``build/progsrc/smileapi``.
By default, it is built as a shared library ``libSMILEapi.so`` 
(``SMILEapi.dll`` on Windows). A static library can be built instead
by setting the CMake build flag ``SMILEAPI_STATIC_LINK=ON``. 
Applications linking to SMILEapi need to include the public header 
``progsrc/include/smileapi/SMILEapi.h`` which defines all types and
functions in the API. For details and usage of the API, see the 
in-line documentation in the header file.

openSMILE also comes with Python and C# wrappers around SMILEapi. 
These can be found in ``progsrc/smileapi/python`` and
``progsrc/smileapi/dotnet``, respectively. The Android and iOS 
sample applications (``progsrc/android-template`` and 
``progsrc/ios-template``) are both based on the SMILEapi, as well,
and can serve as a reference on how to call SMILEapi in mobile apps.

.. note::

    If you intend to use openSMILE from within Python and don't require 
    the advanced, low-level functionality that SMILEapi provides, we 
    highly recommend to check out the standalone 
    `opensmile <https://github.com/audeering/opensmile-python>`_
    Python package. It comes as a pip-installable Python module and 
    provides an easier-to-use interface with e.g. support for NumPy 
    types.


.. _feature-names:

Feature names
-------------

openSMILE follows a strict naming scheme for features (data fields).
Each component (except the sink components) assigns names to its output
fields. All cDataProcessor descendants have two options to control the
naming behaviour, namely ‘nameAppend’ and ‘copyInputName’. ‘nameAppend’
specifies a suffix which is appended to the field name of the previous
level. A ‘–’ is inserted between the two names (if ‘nameAppend’ is not
empty or (null)). ‘copyInputName’ controls whether the input name is
copied and the suffix ‘nameAppend’ and any internal hard-coded names are
appended (if it is set to 1), or if the input field name is discarded
and only the component’s internal names and an appended suffix are used.

The field naming scheme is illustrated by the following example. Let’s
assume you start with an input field ‘pcm’. If you then compute delta
regression coefficients from it, you end up with the name ‘pcm-de’. If
you apply functionals (extreme values max and min only), then you will
end up with two new fields: ‘pcm-de-max’ and ‘pcm-de-min’.
Theoretically, if the ‘copyInputName’ is always set, and a suitable
suffix to append is specified, the complete processing chain can be
deducted from the field name. In practice, however, this would lead to
quite long and redundant feature names, since most speech and music
features base on framing, windowing, and spectral transformation. Thus,
most of these components do not append anything to the input name and do
only copy the input name. In order to discard the ‘pcm’ from the wave
input level, components that compute features such as mfcc, pitch,
etc. discard the input name and use only a hard-coded name or a name
controlled via ‘nameAppend’.


Feature extraction algorithms
-----------------------------

As you might have noted, this document does not describe details of the
feature extraction algorithms implemented in openSMILE. There are two
resources to get more details on the algorithms:

#. Read the source!

#. Read the book *Real-time Speech and Music Classification by Large
   Audio Feature Space Extraction* by F. Eyben published by
   Springer [6]_ (eBook ISBN: 978-3-319-27299-3). All important
   algorithms are described in detail there and a precise and most
   up-to-date summary of standard acoustic parameter sets up to ComParE
   2013 and GeMAPS is given. It is also a good reading for people who
   are new to the field of audio analysis and machine learning for
   audio.

.. [6]
   http://www.springer.com/de/book/9783319272986