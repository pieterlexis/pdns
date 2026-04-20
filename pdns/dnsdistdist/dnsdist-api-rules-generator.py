#!/usr/bin/python3
"""Load action and selector definitions and generates OpenAPI components."""

import sys

import yaml

OUTFILE="dnsdist-api.yml"

def get_definitions_from_file(def_file):
    with open(def_file, "rt", encoding="utf-8") as fd:
        definitions = yaml.safe_load(fd.read())
        return definitions

def get_cpp_object_name(name, is_class=True):
    object_name = ""
    capitalize = is_class
    for char in name:
        if char == "-":
            capitalize = True
            continue
        if capitalize:
            char = char.upper()
            capitalize = False
        object_name += char

    return object_name


def get_cpp_parameter_name(name):
    return get_cpp_object_name(name, is_class=False)

def generate_actions_api_objects(definitions, response=False):
    allActions = list()
    suffix = "ResponseAction" if response else "Action"
    for action in definitions:
        actionDef = dict()
        actionDef["name"] = f'{action["name"]}{suffix}'


def main():
    if len(sys.argv) != 1:
        print(f"Usage: {sys.argv[0]}")
        sys.exit(1)

    definitions = get_definitions_from_file("dnsdist-actions-definitions.yml")
    generate_actions_api_objects(definitions)
