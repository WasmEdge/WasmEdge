#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import division, print_function, absolute_import, unicode_literals
from posixpath import lexists
import shutil
import sys
import argparse
from os.path import expanduser, join, dirname, abspath, exists, islink, lexists, isdir
from os import (
    getenv,
    listdir,
    makedirs,
    mkdir,
    readlink,
    remove,
    getpid,
    symlink,
)
import tempfile
import tarfile
import zipfile
import platform
import subprocess
import re
import logging

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
            value = tp
        if value.__traceback__ is not tb:
            raise value.with_traceback(tb)
        raise value

else:
    exec("def reraise(tp, value=None, tb=None):\n    raise tp, value, tb\n")

    import urllib

    download_url = urllib.urlretrieve


def show_progress(block_num, block_size, total_size):
    downloaded = block_num * block_size

    print(
        end=(
            "\r|%-60s|" % ("=" * int(60 * downloaded / (total_size)))
            + "%6.2f %%" % (downloaded / (total_size) * 100)
        )
    )

    if downloaded < total_size:
        pass
    else:
        print("Downloaded")


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


def extract_archive(
    from_path, ipath, to_path=None, remove_finished=False, env_file_path=None
):
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

    logging.debug("Writing installed files to %s file", env_file_path)
    with open(env_file_path, "a") as env_file:
        for filename in files_extracted:
            fname = filename.replace(CONST_ipkg, ipath)
            if ipath not in fname:
                fname = join(ipath, fname)
            env_file.write("#" + fname + "\n")
            logging.debug("Appending:%s", fname)

    if remove_finished:
        remove(from_path)


# https://stackoverflow.com/questions/1868714/
# how-do-i-copy-an-entire-directory-of-files-
# into-an-existing-directory-using-pyth
def copytree(src, dst, symlinks=False, ignore=None):
    if not exists(dst):
        makedirs(dst)
        shutil.copystat(src, dst)
    lst = listdir(src)
    if ignore:
        excl = ignore(src, lst)
        lst = [x for x in lst if x not in excl]
    for item in lst:
        s = join(src, item)
        d = join(dst, item)
        if symlinks and islink(s):
            if lexists(d):
                remove(d)
            symlink(readlink(s), d)
        elif isdir(s):
            copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)


class VersionString:
    def __init__(self, version):
        self.version = version

    def __str__(self):
        return self.version

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
            a = self._preprocess(
                self.version.split("rc")[0].strip("-"), separator, ignorecase
            )
            b = b = self._preprocess(version2, separator, ignorecase)
            if ((a > b) - (a < b)) == 0:
                return -1
            else:
                return (a > b) - (a < b)
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

WASMEDGE = "WasmEdge"
WASMEDGE_UNINSTALLER = "WasmEdge_Uninstaller"
TENSORFLOW = "tensorflow"
TENSORFLOW_DEPS = "tensorflow_deps"
TENSORFLOW_TOOLS = "tensorflow_tools"
IMAGE = "image"
IMAGE_DEPS = "image_deps"
EXTENSIONS = [TENSORFLOW, IMAGE]

DEPENDENCIES = {
    TENSORFLOW: [
        TENSORFLOW_DEPS,
        TENSORFLOW_TOOLS,
    ],
    IMAGE: [
        IMAGE_DEPS,
    ],
}

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

SUPPORTED_EXTENSIONS_VERSION = {
    "Linux" + "x86_64" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "x86_64" + IMAGE: VersionString("0.8.1"),
    "Linux" + "x86_64" + IMAGE_DEPS: VersionString("0.8.2"),
    "Linux" + "amd64" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "amd64" + IMAGE: VersionString("0.8.1"),
    "Linux" + "arm64" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "arm64" + IMAGE: VersionString("0.8.1"),
    "Linux" + "armv8" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "armv8" + IMAGE: VersionString("0.8.1"),
    "Linux" + "aarch64" + TENSORFLOW: VersionString("0.8.1"),
    "Linux" + "aarch64" + IMAGE: VersionString("0.9.1-beta.1"),
    "Darwin" + "x86_64" + TENSORFLOW: VersionString("0.8.1"),
    "Darwin" + "x86_64" + IMAGE: VersionString("0.10.0-alpha.1"),
    "Darwin" + "arm64" + TENSORFLOW: VersionString("0.8.1"),
    # "Darwin" + "arm64" + IMAGE: VersionString("0.8.1"),
    "Darwin" + "arm" + TENSORFLOW: VersionString("0.8.1"),
    # "Darwin" + "arm" + IMAGE: VersionString("0.8.1"),
}

HOME = expanduser("~")
PATH = join(HOME, ".wasmedge")
SHELL = getenv("SHELL", "bash").split("/")[-1]
TEMP_PATH = join(tempfile.gettempdir(), "wasmedge." + str(getpid()))
CONST_shell_config = None
CONST_shell_profile = None
CONST_env = None
CONST_urls = None
CONST_release_pkg = None
CONST_ipkg = None
CONST_lib_ext = None
CONST_env_path = None

try:
    mkdir(TEMP_PATH)
except:
    pass


def set_env(args, compat):
    global CONST_env, CONST_env_path

    CONST_env = """#!/bin/sh
# wasmedge shell setup
# affix colons on either side of $PATH to simplify matching
case :"${1}": in
    *:"{0}/bin":*)
        ;;
    *)
        # Prepending path in case a system-installed wasmedge needs to be overridden
        if [ -n "${1}" ]; then
            export PATH="{0}/bin":$PATH
        else
            export PATH="{0}/bin"
        fi
        ;;
esac
case :"${2}": in
    *:"{0}/lib":*)
        ;;
    *)
        # Prepending path in case a system-installed wasmedge libs needs to be overridden
        if [ -n "${2}" ]; then
            export {2}="{0}":${2}
        else
            export {2}="{0}"
        fi
        ;;
esac
case :"${3}": in
    *:"{0}/lib":*)
        ;;
    *)
        if [ -n "${3}" ]; then
            export LIBRARY_PATH="{0}/lib":$LIBRARY_PATH
        else
            export LIBRARY_PATH="{0}/lib"
        fi
        ;;
esac
case :"${4}": in
    *:"{0}/include":*)
        ;;
    *)
        if [ -n "${4}" ]; then
            export C_INCLUDE_PATH="{0}/include":$C_INCLUDE_PATH
        else
            export C_INCLUDE_PATH="{0}/include"
        fi
        ;;
esac
case :"${5}": in
    *:"{0}/include":*)
        ;;
    *)
        if [ -n "${5}" ]; then
            export CPLUS_INCLUDE_PATH="{0}/include":$CPLUS_INCLUDE_PATH
        else
            export CPLUS_INCLUDE_PATH="{0}/include"
        fi
        ;;
esac
# Please do not edit comments below this for uninstallation purpose
""".format(
        args.path,
        "PATH",
        compat.ld_library_path,
        "LIBRARY_PATH",
        "C_INCLUDE_PATH",
        "CPLUS_INCLUDE_PATH",
    )

    try:
        mkdir(args.path)
    except:
        pass
    CONST_env_path = join(args.path, "env")
    mode = "w+" if not exists(CONST_env_path) else "w"
    with open(CONST_env_path, mode) as env:
        env.write(CONST_env)


def shell_configure(args):

    global CONST_shell_profile, CONST_shell_config

    source_string = "\n. {0}\n".format(join(args.path, "env"))

    if ("bash" in SHELL) or ("zsh" in SHELL):

        CONST_shell_config = join(HOME, "." + SHELL + "rc")

        if "zsh" in SHELL:
            CONST_shell_profile = join(HOME, "." + "zprofile")
        else:
            CONST_shell_profile = join(HOME, "." + SHELL + "_profile")

        if not exists(CONST_shell_config):
            open(CONST_shell_config, "a").close()

        write_shell = False
        with open(CONST_shell_config, "r") as shell_config:
            if source_string not in shell_config.read():
                write_shell = True

        if write_shell:
            with open(CONST_shell_config, "a") as shell_config:
                shell_config.write(source_string)
            write_shell = False

        if exists(CONST_shell_profile):
            with open(CONST_shell_profile, "r") as shell_profile:
                if source_string not in shell_profile.read():
                    write_shell = True
            if write_shell:
                with open(CONST_shell_profile, "a") as shell_profile:
                    shell_profile.write(source_string)
                write_shell = False
    else:
        logging.error("Unknown shell found")
        return -1

    print("shell configuration updated")
    return 0


def install_image_extension(args, compat):
    global CONST_release_pkg

    if not get_remote_version_availability(
        "second-state/WasmEdge-image", args.image_version
    ):
        logging.error(
            "Image extension version incorrect: {0}".format(args.image_version)
        )
        return -1
    if compat.prefix() + IMAGE not in SUPPORTED_EXTENSIONS_VERSION:
        logging.error("Image extensions not compatible: {0}".format(compat.prefix()))
        return -1
    elif (
        SUPPORTED_EXTENSIONS_VERSION[compat.prefix() + IMAGE].compare(
            args.image_version
        )
        > 0
    ):
        logging.error(
            "Min image extensions version: {0}".format(
                SUPPORTED_EXTENSIONS_VERSION[compat.prefix() + IMAGE],
            )
        )
        return -1

    print("Downloading image extension")

    image_pkg = "WasmEdge-image-" + args.image_version + "-" + CONST_release_pkg

    download_url(CONST_urls[IMAGE], join(TEMP_PATH, image_pkg), show_progress)

    # Extract archieve
    extract_archive(
        join(TEMP_PATH, image_pkg),
        args.path,
        join(TEMP_PATH, "WasmEdge-image"),
        env_file_path=CONST_env_path,
        remove_finished=True,
    )

    copytree(join(TEMP_PATH, "WasmEdge-image"), args.path)

    if compat.prefix() + IMAGE_DEPS in SUPPORTED_EXTENSIONS_VERSION:
        if (
            SUPPORTED_EXTENSIONS_VERSION[compat.prefix() + IMAGE_DEPS].compare(
                args.image_deps_version
            )
            >= 0
        ):
            print("Installing image deps")
            image_deps_pkg = (
                "WasmEdge-image-deps-"
                + args.image_deps_version
                + "-"
                + "manylinux1_x86_64.tar.gz"
            )
            download_url(
                CONST_urls[IMAGE_DEPS], join(TEMP_PATH, image_deps_pkg), show_progress
            )

            # Extract archieve
            extract_archive(
                join(TEMP_PATH, image_deps_pkg),
                join(args.path, "lib"),
                join(TEMP_PATH, "WasmEdge-image-deps"),
                env_file_path=CONST_env_path,
                remove_finished=True,
            )

            copytree(join(TEMP_PATH, "WasmEdge-image-deps"), join(args.path, "lib"))
        else:
            logging.debug("Image deps not needed: {0}".format(args.image_deps_version))
    else:
        logging.debug("Image deps not needed: {0}".format(compat.prefix()))

    return 0


def set_consts(args, compat):
    global CONST_release_pkg, CONST_ipkg, CONST_lib_ext, CONST_urls
    CONST_release_pkg = compat.release_package
    CONST_ipkg = compat.install_package_name
    CONST_lib_ext = compat.lib_extension
    CONST_urls = {
        WASMEDGE: "https://github.com/WasmEdge/WasmEdge/releases/download/{0}/WasmEdge-{0}-{1}".format(
            args.version, CONST_release_pkg
        ),
        WASMEDGE_UNINSTALLER: "https://raw.githubusercontent.com/WasmEdge/WasmEdge/{0}/utils/uninstall.sh".format(
            args.uninstall_script_tag
        ),
        IMAGE: "https://github.com/second-state/WasmEdge-image/releases/download/{0}/WasmEdge-image-{0}-{1}".format(
            args.image_version, CONST_release_pkg
        ),
        IMAGE_DEPS: "https://github.com/second-state/WasmEdge-image/releases/download/{0}/WasmEdge-image-deps-{0}-{1}".format(
            args.image_deps_version, "manylinux1_x86_64.tar.gz"
        ),
    }


def run_shell_command(cmd):
    try:
        output = subprocess.check_output([cmd], shell=True)
        return output.decode().strip()
    except subprocess.CalledProcessError as e:
        print("Exception on process, rc=", e.returncode, "output=", e.output, e.cmd)

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
        self.release_package = None
        self.install_package_name = None
        self.lib_extension = None
        self.ld_library_path = None

        if self.platform == "Linux":
            self.install_package_name = "WasmEdge-{0}-Linux".format(self.version)
            self.lib_extension = ".so"
            self.ld_library_path = "LD_LIBRARY_PATH"
            if self.machine in ["arm64", "armv8", "aarch64"]:
                self.release_package = "manylinux2014_aarch64.tar.gz"
            elif self.machine in ["x86_64", "amd64"]:
                self.release_package = "manylinux2014_x86_64.tar.gz"
            else:
                reraise(Exception("Unsupported arch: {0}".format(self.machine)))
        elif self.platform == "Darwin":
            self.ld_library_path = "DYLD_LIBRARY_PATH"
            self.install_package_name = "WasmEdge-{0}-Darwin".format(self.version)
            self.release_package = "darwin_{0}.tar.gz".format(self.machine)
            self.lib_extension = ".dylib"

    def __str__(self):
        return "Platform:{0}\nMachine:{1}\nVersion:{2}\nExtensions:{3}".format(
            self.platform, self.machine, self.version, self.extensions
        )

    if sys.version_info[0] == 2:

        def __nonzero__(self):
            return self.bool_overload()

    elif sys.version_info[0] == 3:

        def __bool__(self):
            return self.bool_overload()

    def bool_overload(self):
        if self.platform not in SUPPORTED_PLATFORM_MACHINE:
            reraise(Exception("Unsupported platform: {0}".format(self.platform)))
        elif self.machine not in SUPPORTED_PLATFORM_MACHINE[self.platform]:
            reraise(Exception("Unsupported machine: {0}".format(self.machine)))
        elif self.extensions is not None:
            if (
                self.extensions
                not in SUPPORTED_EXTENSIONS[self.platform + self.machine]
            ):
                reraise(
                    Exception(
                        "Extensions not supported. Supported extensions: {0}".format(
                            SUPPORTED_EXTENSIONS[self.platform + self.machine]
                        )
                    )
                )
        elif (
            self.version.compare(
                version2=SUPPORTED_MIN_VERSION[self.platform + self.machine].version
            )
            < 0
        ):
            reraise(
                Exception(
                    "Version not supported. Min Version: {0}".format(
                        SUPPORTED_MIN_VERSION[self.platform + self.machine].version
                    )
                )
            )
        return True

    def prefix(self):
        return self.platform + self.machine


def main(args):
    global CONST_env_path, CONST_release_pkg, CONST_ipkg, CONST_shell_config, CONST_shell_profile

    compat = Compat(version=args.version, extensions=args.extensions)

    logging.debug("Compat object: %s", compat)
    logging.debug("Temp path: %s", TEMP_PATH)
    logging.debug("CLI Args:")
    logging.debug(args)

    if compat:
        print("Compatible with current configuration")

        set_consts(args, compat)

        # Run uninstaller
        uninstaller_path = join(TEMP_PATH, "uninstall.sh")
        download_url(CONST_urls[WASMEDGE_UNINSTALLER], uninstaller_path)

        print("Running Uninstaller")

        logging.debug(
            run_shell_command("bash {0}  -p {1} -q".format(uninstaller_path, args.path))
        )
        remove(uninstaller_path)

        # If args.path is default then remove it initially
        if PATH in args.path and exists(args.path):
            shutil.rmtree(args.path)

        set_env(args, compat)

        logging.debug("CONST_env_path: %s", CONST_env_path)
        logging.debug("CONST_release_pkg: %s", CONST_release_pkg)
        logging.debug("CONST_ipkg: %s", CONST_ipkg)
        logging.debug("CONST_lib_ext: %s", CONST_lib_ext)
        logging.debug("CONST_utls: %s", CONST_urls)

        if getenv("SHELL") != SHELL:
            logging.warning("SHELL variable not found. Using %s as SHELL", SHELL)

        if shell_configure(args) != 0:
            logging.error("Error in configuring shell")

        logging.debug("CONST_shell_profile: %s", CONST_shell_profile)
        logging.debug("CONST_shell_config: %s", CONST_shell_config)

        print("Downloading WasmEdge")

        # Download WasmEdge
        download_url(
            CONST_urls[WASMEDGE], join(TEMP_PATH, CONST_release_pkg), show_progress
        )

        # Extract archieve
        extract_archive(
            join(TEMP_PATH, CONST_release_pkg),
            args.path,
            join(TEMP_PATH),
            env_file_path=CONST_env_path,
            remove_finished=True,
        )

        print("Installing WasmEdge")
        # Copy the tree
        copytree(join(TEMP_PATH, CONST_ipkg), args.path)

        # Check if wasmedge binary works
        wasmedge_output = run_shell_command(
            "{0}/bin/wasmedge --version".format(args.path)
        )

        if args.version in wasmedge_output:
            print("WasmEdge Successfully installed")
            print("Run:\nsource {0}".format(CONST_shell_config))
        else:
            logging.critical(
                "WasmEdge installation incorrect: {0}".format(wasmedge_output)
            )

        if IMAGE in args.extensions:
            if install_image_extension(args, compat) != 0:
                logging.error("Error in installing image extensions")
            else:
                print("Image extension installed")

        # Cleanup
        shutil.rmtree(TEMP_PATH)
    else:
        reraise(Exception("Incompatible with your machine\n{0}".format(compat)))

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
        default=[],
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
        dest="loglevel",
        required=False,
        action="store_const",
        const=logging.INFO,
        help="Verbosity info",
    )
    parser.add_argument(
        "-D",
        "--debug",
        dest="loglevel",
        required=False,
        action="store_const",
        const=logging.DEBUG,
        help="Verbosity debug",
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
        default=get_latest_github_release("WasmEdge/WasmEdge"),
        help="GitHub tag for uninstall script",
    )
    parser.add_argument(
        "--tf-version",
        dest="tf_version",
        required=False,
        default=get_latest_github_release("WasmEdge/WasmEdge"),
        help="Tensorflow and tensorflow lite version",
    )
    parser.add_argument(
        "--tf-deps-version",
        dest="tf_deps_version",
        required=False,
        default=get_latest_github_release("WasmEdge/WasmEdge"),
        help="Tensorflow and tensorflow lite deps version",
    )
    parser.add_argument(
        "--tf-tools-version",
        dest="tf_tools_version",
        required=False,
        default=get_latest_github_release("WasmEdge/WasmEdge"),
        help="Tensorflow and tensorflow lite tools version",
    )
    parser.add_argument(
        "--image-version",
        dest="image_version",
        required=False,
        default=get_latest_github_release("second-state/WasmEdge-image"),
        help="Image extension version",
    )
    parser.add_argument(
        "--image-deps-version",
        dest="image_deps_version",
        required=False,
        default=get_latest_github_release("second-state/WasmEdge-image"),
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

    logging.basicConfig(format="%(levelname)-8s- %(message)s", level=args.loglevel)

    args.path = abspath(args.path)

    main(args)
