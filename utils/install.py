#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import division, print_function, absolute_import, unicode_literals
import sys
import argparse
from os.path import expanduser, join, splitext, basename, dirname, abspath
from os import remove
import tarfile
import zipfile
import platform
import subprocess
import re

try:
    from future_builtins import ascii, filter, hex, map, oct, zip
except:
    pass

download_url = None

# Define version specific things
if sys.version_info[0] == 3:
    xrange = range

    import urllib.request

    download_url = urllib.request.urlretrieve

    def reraise(tp, value=None, tb=None):
        if value is None:
            value = tp()
        if value.__traceback__ is not tb:
            raise value.with_traceback(tb)
        raise value

else:
    exec("def reraise(tp, value=None, tb=None):\n    raise tp, value, tb\n")

    import urllib

    download_url = urllib.urlretrieve


def _is_tarxz(filename):
    return filename.endswith(".tar.xz")


def _is_tar(filename):
    return filename.endswith(".tar")


def _is_targz(filename):
    return filename.endswith(".tar.gz")


def _is_tgz(filename):
    return filename.endswith(".tgz")


def _is_zip(filename):
    return filename.endswith(".zip")


def extract_archive(from_path, to_path=None, remove_finished=False, env_file_path=None):
    files_extracted = []

    if to_path is None:
        to_path = dirname(from_path)

    if _is_tar(from_path):
        with tarfile.open(from_path, "r") as tar:
            tar.extractall(path=to_path)
            files_extracted = tar.getnames()
    elif _is_targz(from_path) or _is_tgz(from_path):
        with tarfile.open(from_path, "r:gz") as tar:
            tar.extractall(path=to_path)
            files_extracted = tar.getnames()
    elif _is_tarxz(from_path):
        with tarfile.open(from_path, "r:xz") as tar:
            tar.extractall(path=to_path)
            files_extracted = tar.getnames()
    elif _is_zip(from_path):
        with zipfile.ZipFile(from_path, "r") as z:
            z.extractall(to_path)
            files_extracted = z.namelist()
    else:
        reraise(ValueError("Extraction of {} not supported".format(from_path)))

    with open(env_file_path, "a") as env_file:
        for filename in files_extracted:
            env_file.write("#" + abspath(filename) + "\n")

    if remove_finished:
        remove(from_path)


class VersionString:
    def __init__(self, version):
        self.version = version

    def _preprocess(self, v, separator, ignorecase):
        if ignorecase:
            v = v.lower()
        return [
            int(x)
            if x.isdigit()
            else [int(y) if y.isdigit() else y for y in re.findall("\d+|[a-zA-Z]+", x)]
            for x in re.split(separator, v)
        ]

    def compare(self, version2, separator=". |-", ignorecase=True):
        # return 1 if self.version > version2
        # return 0 if self.version == version2
        # return -1 if self.version < version2
        # return False if not comparable
        if "rc" in self.version and not "rc" in version2:
            a = self._preprocess(self.version.split(
                "rc")[0].strip("-"), separator, ignorecase)
            b = b = self._preprocess(version2, separator, ignorecase)
            if ((a > b)-(a < b)) == 0:
                return -1
            else:
                return ((a > b)-(a < b))
        else:
            a = self._preprocess(self.version, separator, ignorecase)
            b = self._preprocess(version2, separator, ignorecase)
            try:
                return (a > b) - (a < b)
            except:
                return False


SUPPORTED_PLATFORM_MACHINE = {
    "Linux": ["x86_64", "amd64", "arm64", "armv8", "aarch64"],
    "Darwin": ["x86_64", "arm64", "arm"],
}

SUPPORTED_MIN_VERSION = {
    "Linux" + "x86_64": VersionString("0.8.0"),
    "Linux" + "amd64": VersionString("0.8.0"),
    "Linux" + "arm64": VersionString("0.8.0"),
    "Linux" + "armv8": VersionString("0.8.1"),
    "Linux" + "aarch64": VersionString("0.8.1"),
    "Darwin" + "x86_64": VersionString("0.8.2"),
    "Darwin" + "arm64": VersionString("0.8.2"),
    "Darwin" + "arm": VersionString("0.8.2"),
}

TENSORFLOW = "tensorflow"
IMAGE = "image"
EXTENSIONS = [TENSORFLOW, IMAGE]

SUPPORTED_EXTENSIONS = {
    "Linux" + "x86_64": EXTENSIONS,
    "Linux" + "amd64": EXTENSIONS,
    "Linux" + "arm64": EXTENSIONS,
    "Linux" + "armv8": EXTENSIONS,
    "Linux" + "aarch64": EXTENSIONS,
    "Darwin" + "x86_64": EXTENSIONS,
    "Darwin" + "arm64": [],
    "Darwin" + "arm": [],
}

SUPPORTED_EXTENSIONS_MIN_VERSION = {
    "Linux" + "x86_64" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "x86_64" + IMAGE: VersionString("0.8.1"),
    "Linux" + "amd64" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "amd64" + IMAGE: VersionString("0.8.1"),
    "Linux" + "arm64" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "arm64" + IMAGE: VersionString("0.8.1"),
    "Linux" + "armv8" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "armv8" + IMAGE: VersionString("0.8.1"),
    "Linux" + "aarch64" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "aarch64" + IMAGE: VersionString("0.8.1"),
    "Darwin" + "x86_64" + TENSORFLOW: VersionString("0.8.1"),
    "Darwin" + "x86_64" + IMAGE: VersionString("0.8.1"),
    "Darwin" + "arm64" + TENSORFLOW: VersionString("0.8.1"),
    "Darwin" + "arm64" + IMAGE: VersionString("0.8.1"),
    "Darwin" + "arm" + TENSORFLOW: VersionString("0.8.1"),
    "Darwin" + "arm" + IMAGE: VersionString("0.8.1"),
}

HOME = expanduser("~")
PATH = join(HOME, ".wasmedge")


def run_shell_command(cmd):
    try:
        output = subprocess.check_output([cmd], shell=True)
        return output.decode().strip()
    except subprocess.CalledProcessError as e:
        print("Exception on process, rc=",
              e.returncode, "output=", e.output, e.cmd)

    return None


def get_latest_github_release(repo):
    return run_shell_command(
        """git ls-remote --refs --tags "https://github.com/{0}.git" |
        cut -d '/' -f 3 |
        awk {1} | sort --version-sort | sed 's/_$//' |
        grep -e '^[0-9]\+.[0-9]\+.[0-9]\+$' |
        tail -1""".format(
            repo,
            "'{ if ($1 ~ /-/) print; else print $0\"_\" ;}'",
        )
    )


def get_remote_version_availability(repo, version):
    output = run_shell_command(
        """git ls-remote --refs --tags "https://github.com/{0}.git" |
        cut -d '/' -f 3 |
        awk {1} | sort --version-sort | sed 's/_$//'""".format(
            repo,
            "'{ if ($1 ~ /-/) print; else print $0\"_\" ;}'",
        )
    )
    if version in output:
        return True
    return False


class Compat:
    def __init__(
        self,
        platform=platform.system(),
        machine=platform.machine(),
        version=None,
        extensions=None,
    ):
        self.platform = platform  # Linux, Darwin
        self.machine = machine  # x86_64, arm
        self.version = VersionString(version)
        self.extensions = extensions

    if sys.version_info[0] == 2:

        def __nonzero__(self):
            return self.bool_overload()

    elif sys.version_info[0] == 3:

        def __bool__(self):
            return self.bool_overload()

    def bool_overload(self):
        if self.platform not in SUPPORTED_PLATFORM_MACHINE:
            reraise(
                Exception("Unsupported platform: {0}".format(self.platform)))
        elif self.machine not in SUPPORTED_PLATFORM_MACHINE[self.platform]:
            reraise(Exception("Unsupported machine: {0}".format(self.machine)))
        elif self.extensions not in SUPPORTED_EXTENSIONS[self.platform + self.machine]:
            reraise(
                Exception(
                    "Extensions not supported. Supported extensions: {0}".format(
                        SUPPORTED_EXTENSIONS[self.platform + self.machine]
                    )
                )
            )
        elif self.version.compare(version2=SUPPORTED_MIN_VERSION[self.platform+self.machine].version) < 0:
            reraise(
                Exception(
                    "Version not supported. Min Version: {0}".format(
                        SUPPORTED_MIN_VERSION[self.platform +
                                              self.machine].version
                    )
                )
            )
        return True


def main(args):
    print(get_latest_github_release("WasmEdge/WasmEdge"))
    print(get_remote_version_availability("WasmEdge/WasmEdge", "0.9.4"))
    compat = Compat(version=args.version, extensions=args.extensions)
    if compat:
        print("Compatible")

    pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="WasmEdge installation, uninstallation and extensions install"
    )
    parser.add_argument(
        "-e",
        "--extension",
        dest="extensions",
        choices=EXTENSIONS.append("all"),
        required=False,
        help="Supported Extensions - {0}".format(EXTENSIONS),
    )
    parser.add_argument(
        "-v",
        "--version",
        dest="version",
        default=get_latest_github_release("WasmEdge/WasmEdge"),
        required=False,
        help="Version for WasmEdge",
    )
    parser.add_argument(
        "-V",
        "--verbose",
        dest="verbose",
        required=False,
        action="store_true",
        help="Verbosity",
    )
    parser.add_argument(
        "-p",
        "--path",
        dest="path",
        required=False,
        default=PATH,
        help="Installation path for WasmEdge",
    )
    parser.add_argument(
        "-r",
        "--remove-old",
        dest="remove_old",
        required=False,
        choices=["yes", "no"],
        help="Run uninstaller script before installing",
    )
    parser.add_argument(
        "-u",
        "--uninstall-script-tag",
        dest="uninstall_script_tag",
        required=False,
        help="GitHub tag for uninstall script",
    )
    parser.add_argument(
        "--tf-version",
        dest="tf_version",
        required=False,
        help="Tensorflow and tensorflow lite version",
    )
    parser.add_argument(
        "--tf-deps-version",
        dest="tf_deps_version",
        required=False,
        help="Tensorflow and tensorflow lite deps version",
    )
    parser.add_argument(
        "--tf-tools-version",
        dest="tf_tools_version",
        required=False,
        help="Tensorflow and tensorflow lite tools version",
    )
    parser.add_argument(
        "--image-version",
        dest="image_version",
        required=False,
        help="Image extension version",
    )
    parser.add_argument(
        "--image-deps-version",
        dest="image_deps_version",
        required=False,
        help="Image Deps version",
    )
    parser.add_argument(
        "--ignore-brew",
        dest="ignore_brew",
        required=False,
        action="store_true",
        help="Ignore brew on macOS",
    )
    args = parser.parse_args()

    main(args)
