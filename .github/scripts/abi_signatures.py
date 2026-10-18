#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""Preserve exported function type identities before layout suppression."""

from collections import namedtuple
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET


def opaque_types():
    text = Path(__file__).with_name("abi-opaque.suppr").read_text()
    blocks = text.split("[suppress_type]")
    names = set()
    for block in blocks[1:]:
        match = re.fullmatch(r"\s*name = ([A-Za-z_][A-Za-z_0-9]*)\s+accessed_through = pointer\s*", block)
        if not match:
            raise ValueError("Opaque policy must contain exact pointer-only type names")
        names.add(match.group(1))
    if not names:
        raise ValueError("Empty opaque policy")
    return names


OPAQUE_TYPES = opaque_types()
TypeReference = namedtuple("TypeReference", "index")


def equivalent(left, right):
    left_roots, left_nodes = left
    right_roots, right_nodes = right
    pending = [(left_roots, right_roots)]
    visited = set()
    while pending:
        a, b = pending.pop()
        if isinstance(a, TypeReference) or isinstance(b, TypeReference):
            if not isinstance(a, TypeReference) or not isinstance(b, TypeReference):
                return False
            pair = (a.index, b.index)
            if pair not in visited:
                visited.add(pair)
                pending.append((left_nodes[a.index], right_nodes[b.index]))
        elif isinstance(a, tuple) or isinstance(b, tuple):
            if not isinstance(a, tuple) or not isinstance(b, tuple) or len(a) != len(b):
                return False
            pending.extend(zip(a, b))
        elif a != b:
            return False
    return True


def signatures(library):
    document = ET.fromstring(subprocess.check_output(["abidw", str(library)]))
    types = {node.get("id"): node for node in document.iter() if node.get("id")}
    scopes = {}

    def visit(node, scope=()):
        if node.get("id"):
            scopes[node.get("id")] = scope
        if node.tag in {"namespace-decl", "class-decl", "union-decl"}:
            scope += (node.get("name", ""),)
        for child in node:
            visit(child, scope)

    visit(document)

    def resolve(identifier):
        seen = set()
        while True:
            if identifier not in types or identifier in seen:
                raise ValueError(f"Unresolved or cyclic ABI typedef: {identifier}")
            seen.add(identifier)
            node = types[identifier]
            if node.tag not in {"typedef-decl", "qualified-type-def"}:
                return identifier
            identifier = node.get("type-id")

    def function_graph(function):
        pending = []
        indices = {}

        def reference(identifier, opaque_pointer=False):
            identifier = resolve(identifier)
            key = (identifier, opaque_pointer)
            if key not in indices:
                indices[key] = len(pending)
                pending.append(key)
            return TypeReference(indices[key])

        def parameters(node):
            return tuple(
                (child.tag, child.get("is-variadic", "no"),
                 reference(child.get("type-id")) if child.get("type-id") else None)
                for child in node if child.tag in {"parameter", "return"}
            )

        roots = parameters(function)
        graph = []
        cursor = 0
        while cursor < len(pending):
            identifier, opaque_pointer = pending[cursor]
            cursor += 1
            node = types[identifier]
            tag = node.tag
            identity = (tag, scopes.get(identifier, ()), node.get("name", ""))
            if tag in {"pointer-type-def", "reference-type-def"}:
                attributes = tuple((key, node.get(key)) for key in
                                   ("kind",) if node.get(key))
                through_pointer = tag == "pointer-type-def"
                value = (tag, attributes, reference(node.get("type-id"), through_pointer))
            elif tag == "function-type":
                value = (tag, parameters(node))
            elif tag == "array-type-def":
                bounds = tuple(
                    (tuple((key, child.get(key)) for key in
                           ("length", "lower-bound", "upper-bound") if child.get(key) is not None),
                     reference(child.get("type-id")) if child.get("type-id") else None)
                    for child in node if child.tag == "subrange"
                )
                value = (tag, reference(node.get("type-id")), bounds)
            elif tag in {"class-decl", "union-decl"}:
                if opaque_pointer and not scopes.get(identifier) and node.get("name") in OPAQUE_TYPES:
                    value = identity
                else:
                    members = tuple(
                        reference(member.get("type-id"))
                        for child in node if child.tag == "data-member"
                        and child.get("static", "no") != "yes"
                        for member in child if member.tag == "var-decl"
                    )
                    bases = tuple(reference(child.get("type-id")) for child in node
                                  if child.tag == "base-class")
                    value = (tag, members, bases)
            elif tag in {"enum-decl", "type-decl"}:
                value = identity
            else:
                raise ValueError(f"Unsupported ABI type node: {tag}")
            graph.append(value)
        return roots, tuple(graph)

    result = {}
    for node in document.iter("function-decl"):
        symbol = node.get("elf-symbol-id")
        if not symbol:
            continue
        signature = function_graph(node)
        symbol = symbol.replace("@@", "@")
        if symbol in result and not equivalent(result[symbol], signature):
            raise ValueError(f"Conflicting type declarations for {symbol}")
        result[symbol] = signature
    if not result:
        raise ValueError("No exported function type declarations found")
    return result


def check_signatures(baseline, candidate):
    old = signatures(baseline)
    new = signatures(candidate)
    changed = sorted(name for name, signature in old.items()
                     if name not in new or not equivalent(signature, new[name]))
    for name in changed:
        print(f"Exported function type identity changed or missing: {name}")
    return not changed
