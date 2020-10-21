"""
Python 3 script to visualize the data flow in openSMILE configuration files as graphviz diagrams.
Can optionally run graphviz to render the generated .dot file as an image (requires graphviz to be installed).

Sample usage:
    python3 conf2dot.py MFCC12_0_D_A.conf MFCC12_0_D_A.png --format png

Run with parameter -h to get a list of all accepted command-line options.
"""

import argparse
import io
import os
import re
import shutil
import subprocess
import sys
from typing import List, Tuple, Dict


class Section():
    def __init__(self, name: str, type: str, **kwargs):
        self.name = name
        self.type = type
        self.properties = {}


class ConfFile():
    def __init__(self, path, command_line_args: Dict[str, str] = {}, **kwargs):
        self._command_line_args = command_line_args
        self._command_line_options = {}
        self.sections = {}
        self._parse(path)

    def _parse_cm_directive(self, s: str) -> str:
        match = re.fullmatch(r"\\cm\[([^\(\{:]*)(\(([^\)]*)\))?(\{(.*)\})?(:(.*))?\]", s)
        if match is None:
            raise ValueError("Error parsing \\cm directive")
        long_option, _, short_option, _, default, _, description = match.groups()
        if default is not None:
            self._command_line_options[long_option] = default
        if long_option in self._command_line_args.keys():
            return self._command_line_args[long_option]
        elif long_option in self._command_line_options.keys():
            return self._command_line_options[long_option]
        else:
            raise ValueError("Could not determine a value for command-line option \"{}\"".format(long_option))

    def _expand_cm(self, line: str) -> str:
        match = re.search(r"\\cm\[[^\]]*\]", line)
        if match is None:
            return line
        else:
            expanded_value = self._parse_cm_directive(match.group())
            return match.string[:match.start()] + expanded_value + match.string[match.end():]

    def _parse_include_directive(self, line: str) -> str:
        match = re.fullmatch(r"\\{(.+)\}", line)
        if not match:
            raise ValueError("Invalid include directive")
        return match.groups()[0]

    def _parse_section_header(self, line: str) -> Section:
        match = re.fullmatch(r"\[(\w+):(\w+)\]", line)
        if not match:
            raise ValueError("Invalid section header")
        section_name, section_type = match.groups()
        return Section(section_name, section_type)

    def _parse_property(self, line: str) -> (str, object):
        property_name, property_value = re.split(r"\s*=\s*", line, maxsplit=1)
        property_value = self._expand_cm(property_value)
        array_elements = re.split(r"\s*;\s*", property_value) # TODO: fix escaped \;
        if len(array_elements) > 1:
            return (property_name, array_elements)
        return (property_name, property_value)

    def _parse(self, path: str):
        current_section = None
        in_multi_line_comment = False
        def parse_line(line: str, source_directory: str):
            nonlocal current_section, in_multi_line_comment
            line = line.strip()        
            if line == "" or line.startswith(";") or line.startswith("//") or line.startswith("#") or line.startswith("%"):
                pass  # skip comments and empty lines
            elif line.startswith("/*"):
                in_multi_line_comment = True
            elif line.endswith("*/"):
                in_multi_line_comment = False
            elif in_multi_line_comment:
                pass
            elif line.startswith(r"\{"): # follow include 
                include_path = self._parse_include_directive(line)
                include_path = self._expand_cm(include_path)
                if not os.path.isabs(include_path):
                    include_path = os.path.join(source_directory, include_path)  
                source_directory = os.path.split(os.path.abspath(include_path))[0]
                if os.path.exists(include_path):
                    with open(include_path, encoding="latin-1") as f:
                        for line in f.readlines():
                            parse_line(line, source_directory)
                else:
                    print("Warning: included file {} not found.".format(include_path))
            elif line.startswith("["):
                section = self._parse_section_header(line)
                if section.name in self.sections.keys():
                    current_section = self.sections[section.name]
                else:
                    self.sections[section.name] = section
                    current_section = section
            else:
                property_name, property_value = self._parse_property(line)
                if current_section is None:
                    raise ValueError("Missing section header")
                current_section.properties[property_name] = property_value
        source_directory = os.path.split(os.path.abspath(path))[0]
        with open(path, encoding="latin-1") as f:
            for line in f:
                parse_line(line, source_directory)


def generate_dot_file(sections: List[Section], f, omit_levels=False):
    def is_data_memory_reader(property_name: str, section: Section) -> bool:
        if property_name.endswith(".dmLevel"):
            if property_name.lower().find("reader") != -1:
                return True
            elif property_name.lower().find("writer") == -1:
                print("Warning: property {} of component {}:{} appears to reference a data memory level but could not be interpreted as reading or writing.".format(property_name, section.name, section.type))
            return False
    def is_data_memory_writer(property_name: str, section: Section) -> bool:
        if property_name.endswith(".dmLevel"):
            if property_name.lower().find("writer") != -1:
                return True
            elif property_name.lower().find("reader") == -1:
                print("Warning: property {} of component {}:{} appears to reference a data memory level but could not be interpreted as reading or writing.".format(property_name, section.name, section.type))
            return False
    def get_levels(property_value: str) -> List[str]:
        if isinstance(property_value, str):
            return [property_value]
        elif isinstance(property_value, list):
            return property_value

    components = [(section.name, section.type) for section in sections.values() if section.type != "cComponentManager"]
    levels = set()
    reads = []
    writes = []

    for section in sections.values():
        for property_name, property_value in section.properties.items():
            if is_data_memory_reader(property_name, section):
                l = get_levels(property_value)
                levels.update(l)
                for level in l:
                    reads.append((section.name, level))
            elif is_data_memory_writer(property_name, section):
                l = get_levels(property_value)
                levels.update(l)
                for level in l:
                    writes.append((section.name, level))

    #for section in sections.values():
    #    for property_name, property_value in section.properties.items():
    #        if not is_data_memory_reader(property_name, section) and not is_data_memory_writer(property_name, section):
    #            if not levels.isdisjoint(get_levels(property_value)):
    #                print("Warning: property {} = {} of component {}:{} appears to reference a data memory level but was not recognized as such.".format(property_name, property_value, section.name, section.type))

    f.write("digraph G {\n")
    for (component_name, component_type) in components:
        f.write('    "nodeComponent' + component_name + '" [\n')
        f.write('        label = "' + component_name + "\\n" + component_type + '"\n')
        f.write('        shape = "box"\n')
        f.write('        fillcolor = "gray85"\n')
        f.write('        style = "filled"\n')
        f.write("    ];\n")
    if not omit_levels:
        for level in levels:
            f.write('    "nodeLevel' + level + '" [\n')
            f.write('        label = "' + level + '"\n')
            f.write('        shape = "ellipse"\n')
            f.write('        fillcolor = "gray95"\n')
            f.write('        style = "filled"\n')
            f.write("    ];\n")
    if not omit_levels:
        for (component_name, level) in writes:
            f.write('    "nodeComponent' + component_name + '" -> "nodeLevel' + level + '";\n')
        for (component_name, level) in reads:
            f.write('    "nodeLevel' + level + '" -> "nodeComponent' + component_name + '";\n')
    else:
        for (write_component_name, write_level) in writes:
            for (read_component_name, read_level) in reads:
                if write_level == read_level:
                    f.write('    "nodeComponent' + write_component_name + '" -> "nodeComponent' + read_component_name + '";\n')
    f.write("}\n")


def ensure_graphviz_available():
    if shutil.which("dot") is None:
        print("Error: could not find graphviz. For output formats other than \"dot\", graphviz needs to be installed and on the path.", file=sys.stderr)
        sys.exit(1)


def main(args: argparse.Namespace):
    conf_file = ConfFile(args.input)
    if args.format.lower() == "dot":
        with open(args.output, "w") as f:    
            generate_dot_file(conf_file.sections, f, args.omit_levels)
    else:
        ensure_graphviz_available()
        with io.StringIO() as f:
            generate_dot_file(conf_file.sections, f, args.omit_levels)
            f.seek(0)
            dot_file = f.read()
        subprocess.run(["dot", "-T", args.format, "-o", args.output], input=dot_file, universal_newlines=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generates graphviz dot files from openSMILE configuration files")
    parser.add_argument("input", metavar="input", type=str, help="path to openSMILE configuration file")
    parser.add_argument("output", metavar="output", type=str, help="path where the output dot file should be saved")
    parser.add_argument("--format", "-f", metavar="format", type=str, default="dot", help="output format, see graphviz usage help")
    parser.add_argument("--omit-levels", action="store_true", help="do not include data memory levels in the graph")
    args = parser.parse_args()
    main(args)
