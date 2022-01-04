

# Project -----------------------------------------------------------------

project = 'openSMILE'
copyright = '2013-2022 audEERING GmbH and 2008-2013 TU München, MMK'
author = 'Florian Eyben, Felix Weninger, Martin Wöllmer, Björn Schuller'
# TODO: add script to get version automatically
version = '3.0'
release = '3.0'
title = '{} Documentation'.format(project)


# General -----------------------------------------------------------------

master_doc = 'index'
extensions = []
source_suffix = '.rst'
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'README.rst']
pygments_style = None
extensions = ['sphinxcontrib.bibtex']
bibtex_bibfiles = ['bibliography.bib']
numfig = True


# HTML --------------------------------------------------------------------

html_theme = 'sphinx_audeering_theme'
html_theme_options = {
    'display_version': True,
    'logo_only': True,
    'footer_links': False
}
html_context= {
    'display_github': True,
    'github_host': 'github.com',
    'github_user': 'audeering',
    'github_repo': 'opensmile',
    'conf_py_path': '/doc/sphinx/'
}
html_logo = '_static/images/openSMILE-logoSlogan-white.svg'
html_static_path = ['_static']
html_title = title


# LaTeX -------------------------------------------------------------------

latex_elements = {
    'papersize': 'a4paper',
    # The font size ('10pt', '11pt' or '12pt').
    #
    # 'pointsize': '10pt',
    'preamble': r'''
        \makeatletter
        \renewcommand{\maketitle}{
            \begin{titlepage}
            \begin{center}

            . \\
            \vspace{1cm}

            \includegraphics[width=.4\columnwidth]{logo-opensmile}\\
            \vspace{1.0cm}
            {\Large{open-Source Media Interpretation by Large feature-space Extraction}}\\
            \vspace{1.5cm}
            Version 3.0, \@date\\
            \vspace{1.5cm}

            \textbf{Main authors:} Florian Eyben, Felix Weninger, Martin W{\"o}llmer, Bj{\"o}rn Schuller\\
            \vspace{0.2cm}
            E-mails: fe, fw, mw, bs at audeering.com \\
            \vspace{0.5cm}
            Copyright (C) 2013-2022 by\\
            \vspace{0.2cm}
            {\large{audEERING GmbH}} \\
            \vspace{0.8cm}
            Copyright (C) 2008-2013 by\\
            \vspace{0.2cm}
            {\large{TU M{\"u}nchen, MMK}} \\

            \vspace{3cm}

            \includegraphics[width=.3\columnwidth]{logo-audeering}\\
            \vspace{0.5cm}
            audEERING GmbH \\
            D-82205 Gilching, Germany \\
            \url{http://www.audeering.com/} \\

            \vspace{1.5cm}
            The official openSMILE homepage can be found at:
            \url{http://opensmile.audeering.com/} \\

            \end{center}

            \end{titlepage}
        }
        \makeatother''',
}
latex_additional_files = [
    '_static/images/logo-opensmile.pdf',
    '_static/images/logo-audeering.png'
]

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    (master_doc, 'openSMILE-{}.tex'.format(version), title, author, 'manual'),
]
