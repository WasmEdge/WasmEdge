#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""Compare public function ABI with the latest applicable recorded release."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile

from check_abi import elf_identity, readelf, soname
from select_abi_baselines import select


def download(tag, name, repository, directory):
    subprocess.run(["gh", "release", "download", tag, "--repo", repository,
                    "--pattern", name, "--dir", str(directory)], check=True)
    return Path(directory) / name


def library_from_archive(archive, destination):
    with tarfile.open(archive) as package:
        members = [item for item in package.getmembers()
                   if item.isfile() and item.name == "libwasmedge.so.debug"]
        if len(members) != 1 or members[0].size > 1024 ** 3:
            raise ValueError("Invalid ABI archive")
        with package.extractfile(members[0]) as source, destination.open("wb") as output:
            while chunk := source.read(1024 * 1024):
                output.write(chunk)


def validate_manifest(data, tag, commit, identity, expected_soname):
    if data.get("schema") != 1 or data.get("version") != tag or data.get("commit") != commit:
        raise ValueError("Baseline source identity mismatch")
    if data.get("identity") != list(identity) or data.get("soname") != expected_soname:
        raise ValueError("Platform or SONAME mismatch; transition review required")
    digest = data.get("sha256", "")
    if not isinstance(digest, str) or len(digest) != 64 or any(
        character not in "0123456789abcdef" for character in digest
    ):
        raise ValueError("Invalid baseline checksum")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("version")
    parser.add_argument("--repository", default="WasmEdge/WasmEdge")
    args = parser.parse_args()
    pages = json.loads(subprocess.check_output([
        "gh", "api", "--paginate", "--slurp", f"repos/{args.repository}/releases?per_page=100",
    ], text=True))
    releases = [item for page in pages for item in page]
    inventory = {release["tag_name"]: release for release in releases}
    baseline_release = None
    for entry in reversed(select(releases, args.version)):
        tag = entry["tag"]
        commit = subprocess.check_output(["git", "rev-parse", f"refs/tags/{tag}^{{commit}}"], text=True).strip()
        marker = subprocess.check_output([
            "git", "ls-tree", commit, "--", ".github/abi-baseline-schema",
        ], text=True)
        if marker:
            schema = subprocess.check_output([
                "git", "show", f"{commit}:.github/abi-baseline-schema",
            ], text=True).strip()
            if schema != "1":
                raise ValueError(f"Unsupported baseline schema at {tag}: {schema}")
            assets = inventory[tag].get("assets", [])
            for name in ("WasmEdge-abi-linux-x86_64.json", "WasmEdge-abi-linux-x86_64.tar.gz"):
                count = sum(asset.get("name") == name for asset in assets)
                if count != 1:
                    raise ValueError(f"{tag}: expected exactly one {name}; found {count}")
            baseline_release = (tag, commit)
            break
        else:
            print(f"{tag}: predates baseline recording; NOT CHECKED")
    if baseline_release is None:
        raise ValueError("No recorded baseline; enable this gate after the first baseline release")
    identity = elf_identity(readelf(args.candidate, "--file-header"))
    candidate_soname = soname(readelf(args.candidate, "--dynamic"))
    tag, commit = baseline_release
    print(f"Checking public function ABI against {tag} ({commit})")
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        manifest = download(tag, "WasmEdge-abi-linux-x86_64.json", args.repository, root)
        data = json.loads(manifest.read_text())
        validate_manifest(data, tag, commit, identity, candidate_soname)
        archive = download(tag, "WasmEdge-abi-linux-x86_64.tar.gz", args.repository, root)
        library = root / "baseline.so"
        library_from_archive(archive, library)
        if hashlib.sha256(library.read_bytes()).hexdigest() != data["sha256"]:
            raise ValueError("Baseline checksum mismatch")
        return subprocess.run([
            "python3", str(Path(__file__).with_name("check_abi.py")), "--types-only",
            str(library), str(args.candidate),
        ], check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
