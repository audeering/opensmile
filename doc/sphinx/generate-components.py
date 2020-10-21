import argparse
import json
import os
import re


def main(args):
    with open(args.json_help_path) as f:
        json_help = json.load(f)

    # Directory storing the results.
    dir_output = os.path.abspath("./_components")
    if not os.path.exists(dir_output):
        os.mkdir(dir_output)

    for component in json_help["components"]:        
        output = generate_component(component, json_help)
        output_path = os.path.join(dir_output, component["name"] + '.rst')
        with open(output_path, "w") as f:
            f.writelines(line + "\n" for line in output)

    output_index = generate_index(json_help)
    with open("./components.txt", "w") as f:
        f.writelines(line + "\n" for line in output_index)


def generate_component(component, json_help):
    config_type = find_config_type(component["name"], json_help)
    output = []
    generate_header(component, output)
    generate_description(component, output)
    generate_type_hierarchy(config_type, json_help, output)
    if config_type["fields"]:
        generate_fields(config_type, output)
        generate_non_component_types(config_type, json_help, output)
    return output


def generate_header(component, output):    
    output.append(".. Auto-generated openSMILE component documentation")
    output.append("")
    output.append(".. Include isonum to use |rarr|")
    output.append(".. include:: <isonum.txt>")
    output.append("")
    output.append(".. _{}:".format(component["name"]))  # label of section
    output.append("")
    output.append(component["name"])
    output.append("-" * len(component["name"]))
    output.append("")


def generate_description(component, output):
    description = clean_rst_output(component["description"])
    # Fix multi paragraph descriptions
    description = re.sub('\n +', '\n\n', description)
    output.append("Description")
    output.append("~~~~~~~~~~~")
    output.append(description)
    output.append("")


def generate_type_hierarchy(config_type, json_help, output):
    output.append("Type hierarchy")
    output.append("~~~~~~~~~~~~~~")    
    ancestors = get_config_type_ancestors(config_type, json_help)
    if len(ancestors) >= 2:
        link = " |rarr| ".join([":ref:`{}`".format(ct) for ct in ancestors])
        output.append(link)
    else:
        output.append("This config type does not inherit from any base type.")
    output.append("")


def generate_fields(config_type, output):
    output.append("Fields")
    output.append("~~~~~~")
    for field in config_type["fields"]:
        name = field["name"]
        if field["dataType"] != "object":
            data_type = field["dataType"]
        else:
            data_type = "object of type :ref:`{}`".format(field["subType"])
        array_specifier = "[]" if field.get("array", False) else ""
        text = "- **{}{}** ({})".format(name, array_specifier, data_type)
        if "default" in field:
            default = clean_rst_output(str(field["default"]))
            text += " [default: {}]".format(default or "NULL")
        output.append(text)
        if field["description"] is not None:
            description = clean_rst_output(field["description"])
            description = clean_rst_items(description)
            output.append("  ")
            output.append("    " + description)
            output.append("  ")


def generate_non_component_types(config_type, json_help, output):
    # There's a known issue with non-component types that are used by
    # more than one component. For these, we generate documentation
    # more than once, leading to Sphinx warnings about duplicated labels.
    sub_types = set(field["subType"]
                    for field in config_type["fields"] 
                    if field["dataType"] == "object")
    component_type_names = [component["name"]
                            for component in json_help["components"]]
    non_component_type_names = sub_types.difference(component_type_names)
    for name in non_component_type_names:
        non_component_type = find_config_type(name, json_help)
        output.append(".. _{}:".format(non_component_type["name"]))
        output.append("")
        output.append(non_component_type["name"])
        output.append("-" * len(non_component_type["name"]))
        output.append("")
        generate_type_hierarchy(non_component_type, json_help, output)
        if non_component_type["fields"]:
            generate_fields(non_component_type, output)
            generate_non_component_types(non_component_type, json_help, output)


def generate_index(json_help):
    output_index = [
        ".. Auto-generated openSMILE components list.",
        "",
        ".. toctree::",
        "    :maxdepth: 1",
        "",
    ]
    component_names = sorted(
        component["name"] for component in json_help["components"]
    )
    for name in component_names:
        output_index.append("    _components/" + name)
    return output_index


def find_config_type(config_type, json_help):
    return next(filter(lambda ct: ct["name"] == config_type,
                       json_help["configTypes"]), None)


def get_config_type_ancestors(config_type, json_help):
    if "parent" not in config_type:
        return [config_type["name"]]
    else:
        config_type_parent = find_config_type(config_type["parent"], json_help)
        anchestors = get_config_type_ancestors(config_type_parent, json_help)
        return anchestors + [config_type["name"]]


def clean_rst_items(text):
    # "Parameters:\n   Parameter a\n   Parameter b"
    # =>
    # "Parameters:\n\n    -   Parameter a\n    -   Parameter b"
    text = text.replace('\n', '\n    -')
    text = re.sub(': ?\n', ':\n\n', text)
    # Fix if there were already "-" included in the string
    text = text.replace('-  -', '-')
    # Fix the numbered listings of functionals
    text = re.sub(r'    -.+\(#\).+\n', '', text)
    text = re.sub(r'-      [0-9]+\.', '#.', text)
    return text


def clean_rst_output(text):
    # Remove white space at beginning and new line at end of string
    text = text.lstrip().rstrip('\n')
    # Escape all special characters to ensure they are not misinterpreted
    text = re.sub(r'([^A-Za-z0-9\s])', r'\\\1', text)
    return text


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description=(
        "Generate Sphinx documentation on components "
        "from the output of SMILExtract -exportHelp"
    ))
    arg_parser.add_argument(
        "json_help_path",
        type=str,
        help="path to a JSON file containing the output of SMILExtract "
             "-exportHelp"
    )
    main(arg_parser.parse_args())
