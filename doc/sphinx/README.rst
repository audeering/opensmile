OpenSMILE Documentation
=======================

This directory holds the documentation of openSMILE in reStructuredText_
format. It can be used with Sphinx_ to build an HTML and/or PDF version of 
the openSMILE documentation.


Installation
------------

Before generating the documentation, you need to install the required Python
packages. The best way to do this is in a virtual environment and using
``pip``:

.. code-block:: bash

    virtualenv \
        --python="/usr/bin/python3" \
        --no-site-packages \
        "${HOME}/.envs/opensmile-doc"
    source "${HOME}/.envs/opensmile-doc/bin/activate"
    pip install -r requirements.txt.lock


Usage
-----

You can use the Makefile to generate the documentation:

.. code-block:: bash

    # Clean up
    make clean
    # Build HTML documentation under _build/html/
    make html
    # Build PDF documentation under _build/latex/
    make latexpdf

Or alternatively directly type in the commands:

.. code-block:: bash

    # Clean up
    rm -rf _build
    # Build HTML documentation under _build/html/
    sphinx-build -b html -d ./_build/doctrees . ./_build/html/
    # Build PDF documentation under _build/latex/
    sphinx-build -b latex -d ./_build/doctrees . ./_build/latex/
    make -C _build/latex all-pdf


Component documentation
-----------------------

To include openSMILE component documentation in the documentation,
perform the following steps:

#.  Build openSMILE. Note: the build flags you set determine which components
    get included in your build. The documentation you generate in the next 
    step will only contain documentation on the components that were included 
    in the build.

#.  Export openSMILE component help in JSON format by running:

    .. code-block:: bash

        SMILExtract -exportHelp > help.json

#.  Generate Sphinx component help by running:

    .. code-block:: bash

        python generate-components.py help.json

There should now be a ``components.txt`` file and a ``_components`` folder 
with the generated documentation sources. Run ``make html`` or 
``make latexpdf`` as described in the previous section to generate the full
documentation.


.. _reStructuredText: http://docutils.sourceforge.net/rst.html
.. _Sphinx: http://www.sphinx-doc.org
