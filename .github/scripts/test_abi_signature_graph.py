#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import unittest
from unittest.mock import patch

from abi_signatures import TypeReference, equivalent, signatures


def parse(declarations, argument="record"):
    xml = f"""<abi-corpus><abi-instr>
    <type-decl id="int" name="int"/>
    <class-decl id="vm" name="WasmEdge_VMContext"/>
    <class-decl id="configure" name="WasmEdge_ConfigureContext"/>
    <pointer-type-def id="pointer" type-id="vm"/>
    {declarations}
    <function-decl name="api" elf-symbol-id="api">
      <parameter type-id="{argument}"/><return type-id="int"/>
    </function-decl></abi-instr></abi-corpus>"""
    with patch("abi_signatures.subprocess.check_output", return_value=xml.encode()):
        return signatures("fixture")


class GraphTests(unittest.TestCase):
    def test_shared_and_duplicated_pointer_nodes(self):
        record = '<class-decl id="record" name="Public"><data-member><var-decl type-id="pointer"/></data-member><data-member><var-decl type-id="REPLACE"/></data-member></class-decl>'
        shared = parse(record.replace("REPLACE", "pointer"))["api"]
        duplicated = parse('<pointer-type-def id="copy" type-id="vm"/>' + record.replace("REPLACE", "copy"))["api"]
        aliased = parse('<typedef-decl id="alias" name="Handle" type-id="vm"/><pointer-type-def id="copy" type-id="alias"/>' + record.replace("REPLACE", "copy"))["api"]
        self.assertTrue(equivalent(shared, duplicated))
        self.assertTrue(equivalent(duplicated, shared))
        self.assertTrue(equivalent(shared, aliased))
        changed = parse('<pointer-type-def id="copy" type-id="configure"/>' + record.replace("REPLACE", "copy"))["api"]
        self.assertFalse(equivalent(shared, changed))

    def test_equivalent_recursive_graphs(self):
        single = ((TypeReference(0),), (("node", TypeReference(0)),))
        doubled = ((TypeReference(0),), (("node", TypeReference(1)),
                                        ("node", TypeReference(0))))
        changed = ((TypeReference(0),), (("node", TypeReference(1)),
                                       ("different", TypeReference(0))))
        self.assertTrue(equivalent(single, doubled))
        self.assertFalse(equivalent(single, changed))

    def test_nested_member_identity(self):
        record = '<class-decl id="record" name="Public"><data-member><var-decl type-id="pointer"/></data-member></class-decl>'
        changed = '<pointer-type-def id="other" type-id="configure"/>' + record.replace('type-id="pointer"', 'type-id="other"')
        self.assertNotEqual(parse(record), parse(changed))

    def test_callback_variadic_and_identity(self):
        callback = '<function-type id="callback"><parameter type-id="pointer"/>{}<return type-id="int"/></function-type>'
        record = '<pointer-type-def id="cbptr" type-id="callback"/><class-decl id="record" name="Public"><data-member><var-decl type-id="cbptr"/></data-member></class-decl>'
        plain = callback.format("") + record
        variadic = callback.format('<parameter is-variadic="yes"/>') + record
        self.assertNotEqual(parse(plain), parse(variadic))
        changed = '<pointer-type-def id="other" type-id="configure"/>' + plain.replace('type-id="pointer"', 'type-id="other"')
        self.assertNotEqual(parse(plain), parse(changed))

    def test_array_ids_are_not_abi(self):
        array = '<array-type-def id="array" type-id="int"><subrange id="range1" type-id="index1" length="4"/></array-type-def><type-decl id="index1" name="unsigned long"/>'
        renamed = array.replace("range1", "range99").replace("index1", "index99")
        self.assertEqual(parse(array, "array"), parse(renamed, "array"))
        self.assertNotEqual(parse(array, "array"), parse(array.replace('length="4"', 'length="5"'), "array"))

    def test_recursive_record_terminates(self):
        record = '<pointer-type-def id="next" type-id="record"/><class-decl id="record" name="Public"><data-member><var-decl type-id="next"/></data-member></class-decl>'
        self.assertEqual(parse(record), parse(record))


if __name__ == "__main__":
    unittest.main()
