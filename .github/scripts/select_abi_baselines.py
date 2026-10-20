#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""Select earlier published releases using Debian version ordering."""

import functools
import re
import subprocess


def normalize(tag):
    if not isinstance(tag, str):
        raise ValueError("Release tag must be a string")
    value = re.sub(r"^(?:release[-_/]|v)(?=\d)", "", tag, flags=re.IGNORECASE)
    value = re.sub(
        r"(?<=[0-9])[-._]?(alpha|beta|preview|pre|rc)(?=\d|[.-]|$)",
        lambda match: "~" + match.group(1).lower(), value, flags=re.IGNORECASE,
    )
    if not re.fullmatch(r"\d[0-9A-Za-z.+~:-]*", value):
        raise ValueError(f"Unsupported release tag: {tag!r}; provide an explicit mapping")
    result = subprocess.run(["dpkg", "--validate-version", value], capture_output=True)
    if result.returncode:
        raise ValueError(f"Invalid Debian version after normalization: {tag!r}")
    return value


@functools.lru_cache(maxsize=None)
def compare(left, right):
    for operator, result in (("lt", -1), ("gt", 1)):
        status = subprocess.run(
            ["dpkg", "--compare-versions", left, operator, right], check=False
        ).returncode
        if status == 0:
            return result
        if status != 1:
            raise ValueError("dpkg version comparison failed")
    return 0


def select(releases, candidate, mappings=None):
    mappings = mappings or {}
    description = re.fullmatch(r"(.+)-[1-9]\d*-g[0-9a-f]+", candidate)
    base = description.group(1) if description else candidate
    ceiling = normalize(mappings.get(base, base))
    selected = {}
    for release in releases:
        tag = release["tag_name"]
        if not isinstance(tag, str) or any(
            type(release[field]) is not bool for field in ("draft", "prerelease")
        ):
            raise ValueError("Invalid release metadata")
        if release["draft"] or release["prerelease"]:
            continue
        normalized = normalize(mappings.get(tag, tag))
        order = compare(normalized, ceiling)
        if order > 0 or (order == 0 and not description):
            continue
        selected[tag] = {"tag": tag, "version": normalized}
    result = sorted(
        selected.values(),
        key=functools.cmp_to_key(lambda a, b: compare(a["version"], b["version"])),
    )
    if not result:
        raise ValueError("No earlier published stable releases; baseline review required")
    return result
