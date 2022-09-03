#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import (
    division,
    print_function,
    absolute_import,
    unicode_literals,
    with_statement,
)
from contextlib import contextmanager
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


@contextmanager
def opened_w_error(filename, mode="r"):
    try:
        f = open(filename, mode)
    except IOError as err:
        logging.critical("Error opening file: %s error: %s", filename, err.strerror)
        yield None
    else:
        try:
            yield f
        finally:
            f.close()


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
    with opened_w_error(env_file_path, "a") as env_file:
        if env_file is not None:
            for filename in files_extracted:
                fname = filename.replace(CONST_ipkg, ipath)

                # Skip if it ends with "wasmedge" as it is going to be removed at a later stage
                if fname.endswith("wasmedge"):
                    continue

                # replace wasmedge folder name with include
                fname = fname.replace("include/wasmedge", "include")
                if "Plugin" in fname:
                    if ipath not in fname:
                        fname = join(ipath, "plugin", fname)
                    else:
                        # replace lib or lib64 wasmedge name with plugin
                        fname = fname.replace("lib64/wasmedge", "plugin").replace(
                            "lib/wasmedge", "plugin"
                        )
                else:
                    if ipath not in fname:
                        fname = join(ipath, fname)
                # Don't append system directories
                if (
                    args.path != abspath(PATH)
                    and fname.startswith("/usr")
                    and isdir(fname)
                ):
                    continue
                env_file.write("#" + fname + "\n")
                logging.debug("Appending:%s", fname)
        else:
            logging.warning("Unable to write to env file")

    if remove_finished:
        remove(from_path)


# https://stackoverflow.com/questions/1868714/
# how-do-i-copy-an-entire-directory-of-files-
# into-an-existing-directory-using-pyth
def copytree(src, dst, symlinks=True, ignore=None):
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
TENSORFLOW_LITE = "tensorflow_lite"
TENSORFLOW_DEPS = "tensorflow_deps"
TENSORFLOW_LITE_DEPS = "tensorflow_lite_deps"
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
    "Linux" + "aarch64" + TENSORFLOW: VersionString("0.9.1-beta.1"),
    "Linux" + "aarch64" + IMAGE: VersionString("0.9.1-beta.1"),
    "Darwin" + "x86_64" + TENSORFLOW: VersionString("0.10.0-alpha.1"),
    "Darwin" + "x86_64" + IMAGE: VersionString("0.10.0-alpha.1"),
    "Darwin" + "arm64" + TENSORFLOW: VersionString("0.10.0-alpha.1"),
    # "Darwin" + "arm64" + IMAGE: VersionString("0.8.1"),
    "Darwin" + "arm" + TENSORFLOW: VersionString("0.10.0-alpha.1"),
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
CONST_lib_dir = "lib"

try:
    mkdir(TEMP_PATH)
except:
    pass


def set_env(args, compat):
    global CONST_env, CONST_env_path, CONST_lib_dir
    other_lib_dir = None

    if CONST_lib_dir == "lib64":
        other_lib_dir = "lib"
    else:
        other_lib_dir = "lib64"

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
    *:"{0}/{6}":*)
        ;;
    *)
        # Prepending path in case a system-installed wasmedge libs needs to be overridden
        if [ -n "${2}" ]; then
            export {2}="{0}/{6}":${2}
        else
            export {2}="{0}/{6}"
        fi
        ;;
esac
case :"${3}": in
    *:"{0}/{6}":*)
        ;;
    *)
        if [ -n "${3}" ]; then
            export LIBRARY_PATH="{0}/{6}":$LIBRARY_PATH
        else
            export LIBRARY_PATH="{0}/{6}"
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
if [ -d {0}/{7} ];then
    case :"${2}": in
        *:"{0}/{7}":*)
            ;;
        *)
            # Prepending path in case a system-installed wasmedge libs needs to be overridden
            if [ -n "${2}" ]; then
                export {2}="{0}/{7}":${2}
            else
                export {2}="{0}/{7}"
            fi
            ;;
    esac
    case :"${3}": in
    *:"{0}/{7}":*)
        ;;
    *)
        if [ -n "${3}" ]; then
            export LIBRARY_PATH="{0}/{7}":$LIBRARY_PATH
        else
            export LIBRARY_PATH="{0}/{7}"
        fi
        ;;
    esac
fi
# Please do not edit comments below this for uninstallation purpose
""".format(
        args.path,
        "PATH",
        compat.ld_library_path,
        "LIBRARY_PATH",
        "C_INCLUDE_PATH",
        "CPLUS_INCLUDE_PATH",
        CONST_lib_dir,
        other_lib_dir,
    )

    try:
        mkdir(args.path)
        mkdir(join(args.path, "plugin"))
    except:
        pass
    CONST_env_path = join(args.path, "env")
    mode = "w+" if not exists(CONST_env_path) else "w"
    with opened_w_error(CONST_env_path, mode) as env:
        if env is not None:
            env.write(CONST_env)
        else:
            logging.error("Not able to write to env file")


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
        with opened_w_error(CONST_shell_config, "r") as shell_config:
            if shell_config is not None:
                if source_string not in shell_config.read():
                    write_shell = True

        if write_shell:
            with opened_w_error(CONST_shell_config, "a") as shell_config:
                if shell_config is not None:
                    shell_config.write(source_string)
            write_shell = False

        if exists(CONST_shell_profile):
            with opened_w_error(CONST_shell_profile, "r") as shell_profile:
                if shell_profile is not None:
                    if source_string not in shell_profile.read():
                        write_shell = True
            if write_shell:
                with opened_w_error(CONST_shell_profile, "a") as shell_profile:
                    if shell_profile is not None:
                        shell_profile.write(source_string)
                write_shell = False
    else:
        logging.error("Unknown shell found")
        return -1

    print("shell configuration updated")
    return 0


def install_image_extension(args, compat):
    global CONST_release_pkg, CONST_lib_dir

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

    wasmedge_image_temp = join(TEMP_PATH, "WasmEdge-image")
    for dir in listdir(wasmedge_image_temp):
        wasmedge_image_temp_dir = join(wasmedge_image_temp, dir)
        for file in listdir(wasmedge_image_temp_dir):
            if (
                isdir(join(wasmedge_image_temp_dir, file))
                and args.path == abspath(PATH)
                and "wasmedge" == file
            ):
                copytree(
                    join(wasmedge_image_temp_dir, file), join(args.path, "include")
                )
            elif CONST_lib_ext in file:
                if isdir(join(args.path, CONST_lib_dir)):
                    shutil.move(
                        join(wasmedge_image_temp_dir, file),
                        join(args.path, CONST_lib_dir, file),
                    )
                else:
                    shutil.move(
                        join(wasmedge_image_temp_dir, file),
                        join(args.path, "lib", file),
                    )
            elif isdir(join(wasmedge_image_temp_dir, file)):
                copytree(
                    join(wasmedge_image_temp_dir, file),
                    join(args.path, file),
                )
            else:
                shutil.move(
                    join(wasmedge_image_temp_dir, file),
                    join(args.path, "bin", file),
                )

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
                join(args.path, CONST_lib_dir),
                join(TEMP_PATH, "WasmEdge-image-deps"),
                env_file_path=CONST_env_path,
                remove_finished=True,
            )

            copytree(
                join(TEMP_PATH, "WasmEdge-image-deps"), join(args.path, CONST_lib_dir)
            )

            for file in listdir(join(args.path, CONST_lib_dir)):
                if ("jpeg" not in file) and ("png" not in file):
                    continue
                try:
                    # check if it contains any digits
                    if not any(i.isdigit() for i in file):
                        continue
                    if compat.platform == "Linux":
                        name, version = file.split(CONST_lib_ext, 1)
                        no_v_env_path = join(
                            args.path,
                            CONST_lib_dir,
                            name + CONST_lib_ext,
                        )
                        symlink(
                            join(args.path, CONST_lib_dir, file),
                            no_v_env_path,
                        )
                        single_v_env_path = join(
                            args.path,
                            CONST_lib_dir,
                            name + CONST_lib_ext + "." + version.split(".")[1],
                        )
                        symlink(
                            join(args.path, CONST_lib_dir, file),
                            single_v_env_path,
                        )
                        double_v_env_path = join(
                            args.path,
                            CONST_lib_dir,
                            name
                            + CONST_lib_ext
                            + "."
                            + version.split(".")[1]
                            + "."
                            + version.split(".")[2],
                        )
                        symlink(
                            join(args.path, CONST_lib_dir, file),
                            double_v_env_path,
                        )
                        no_v_png_path = None
                        if "png16" in name:
                            no_v_png_path = join(
                                args.path,
                                CONST_lib_dir,
                                name.split("16")[0] + CONST_lib_ext,
                            )
                            symlink(
                                join(args.path, CONST_lib_dir, file),
                                no_v_png_path,
                            )
                    elif compat.platform == "Darwin":
                        name, version = file.split(CONST_lib_ext, 1)[0].split(".", 1)
                        no_v_env_path = join(
                            args.path,
                            CONST_lib_dir,
                            name + CONST_lib_ext,
                        )
                        symlink(
                            join(args.path, CONST_lib_dir, file),
                            no_v_env_path,
                        )
                        single_v_env_path = join(
                            args.path,
                            CONST_lib_dir,
                            name + "." + version.split(".")[0] + CONST_lib_ext,
                        )
                        symlink(
                            join(args.path, CONST_lib_dir, file),
                            single_v_env_path,
                        )
                        double_v_env_path = join(
                            args.path,
                            CONST_lib_dir,
                            name
                            + "."
                            + version.split(".")[0]
                            + "."
                            + version.split(".")[1]
                            + CONST_lib_ext,
                        )
                        symlink(
                            join(args.path, CONST_lib_dir, file),
                            double_v_env_path,
                        )
                        no_v_png_path = None
                        if "png16" in name:
                            no_v_png_path = join(
                                args.path,
                                CONST_lib_dir,
                                name.split("16")[0] + CONST_lib_ext,
                            )
                            symlink(
                                join(args.path, CONST_lib_dir, file),
                                no_v_png_path,
                            )
                    else:
                        reraise(
                            Exception(
                                "Functionality not implemented for platform {0}".format(
                                    compat.platform
                                )
                            )
                        )
                    with opened_w_error(CONST_env_path, "a") as env_file:
                        if env_file is not None:
                            env_file.write("#" + no_v_env_path + "\n")
                            logging.debug("Appending:%s", no_v_env_path)
                            env_file.write("#" + single_v_env_path + "\n")
                            logging.debug("Appending:%s", single_v_env_path)
                            env_file.write("#" + double_v_env_path + "\n")
                            logging.debug("Appending:%s", double_v_env_path)
                            if no_v_png_path is not None:
                                env_file.write("#" + no_v_png_path + "\n")
                                logging.debug("Appending:%s", no_v_png_path)
                        else:
                            logging.error(
                                "Not able to append installed files to env file"
                            )

                except Exception as e:
                    logging.critical(e)

        else:
            logging.debug("Image deps not needed: {0}".format(args.image_deps_version))
    else:
        logging.debug("Image deps not needed: {0}".format(compat.prefix()))

    return 0


def install_tensorflow_extension(args, compat):
    global CONST_release_pkg, CONST_lib_ext, CONST_lib_dir, CONST_env_path

    if not get_remote_version_availability(
        "second-state/WasmEdge-tensorflow", args.tf_version
    ):
        logging.error(
            "Tensorflow extension version incorrect: {0}".format(args.tf_version)
        )
        return -1
    elif not get_remote_version_availability(
        "second-state/WasmEdge-tensorflow-deps", args.tf_deps_version
    ):
        logging.error(
            "Tensorflow Deps extension version incorrect: {0}".format(
                args.tf_deps_version
            )
        )
        return -1
    elif not get_remote_version_availability(
        "second-state/WasmEdge-tensorflow", args.tf_tools_version
    ):
        logging.error(
            "Tensorflow Tools version incorrect: {0}".format(args.tf_tools_version)
        )
        return -1

    if compat.prefix() + TENSORFLOW not in SUPPORTED_EXTENSIONS_VERSION:
        logging.error(
            "Tensorflow extensions not compatible: {0}".format(compat.prefix())
        )
        return -1
    elif (
        SUPPORTED_EXTENSIONS_VERSION[compat.prefix() + TENSORFLOW].compare(
            args.tf_version
        )
        > 0
    ):
        logging.error(
            "Min tensorflow extensions version: {0}".format(
                SUPPORTED_EXTENSIONS_VERSION[compat.prefix() + TENSORFLOW],
            )
        )
        return -1

    tf_pkg = "WasmEdge-tensorflow-" + args.tf_version + "-" + CONST_release_pkg
    tf_lite_pkg = "WasmEdge-tensorflowlite-" + args.tf_version + "-" + CONST_release_pkg
    tf_deps_pkg = (
        "WasmEdge-tensorflow-deps-TF-" + args.tf_deps_version + "-" + CONST_release_pkg
    )
    tf_deps_lite_pkg = (
        "WasmEdge-tensorflow-deps-TFLite-"
        + args.tf_deps_version
        + "-"
        + CONST_release_pkg
    )
    tf_tools_pkg = (
        "WasmEdge-tensorflow-tools-" + args.tf_tools_version + "-" + CONST_release_pkg
    )

    print("Downloading tensorflow extension")
    download_url(CONST_urls[TENSORFLOW], join(TEMP_PATH, tf_pkg), show_progress)

    print("Downloading tensorflow-lite extension")
    download_url(
        CONST_urls[TENSORFLOW_LITE], join(TEMP_PATH, tf_lite_pkg), show_progress
    )

    print("Downloading tensorflow-deps")
    download_url(
        CONST_urls[TENSORFLOW_DEPS], join(TEMP_PATH, tf_deps_pkg), show_progress
    )

    print("Downloading tensorflow-lite-deps")
    download_url(
        CONST_urls[TENSORFLOW_LITE_DEPS],
        join(TEMP_PATH, tf_deps_lite_pkg),
        show_progress,
    )

    print("Downloading tensorflow-tools extension")
    download_url(
        CONST_urls[TENSORFLOW_TOOLS], join(TEMP_PATH, tf_tools_pkg), show_progress
    )

    # Extract archieve
    extract_archive(
        join(TEMP_PATH, tf_pkg),
        args.path,
        join(TEMP_PATH, "WasmEdge-tensorflow"),
        env_file_path=CONST_env_path,
        remove_finished=True,
    )
    # Extract archieve
    extract_archive(
        join(TEMP_PATH, tf_lite_pkg),
        args.path,
        join(TEMP_PATH, "WasmEdge-tensorflow-lite"),
        env_file_path=CONST_env_path,
        remove_finished=True,
    )
    # Extract archieve
    extract_archive(
        join(TEMP_PATH, tf_deps_pkg),
        join(args.path, CONST_lib_dir),
        join(TEMP_PATH, "WasmEdge-tensorflow-deps", CONST_lib_dir),
        env_file_path=CONST_env_path,
        remove_finished=True,
    )
    # Extract archieve
    extract_archive(
        join(TEMP_PATH, tf_tools_pkg),
        join(args.path, "bin"),
        join(TEMP_PATH, "WasmEdge-tensorflow-tools", "bin"),
        env_file_path=CONST_env_path,
        remove_finished=True,
    )
    # Extract archieve
    extract_archive(
        join(TEMP_PATH, tf_deps_lite_pkg),
        join(args.path, CONST_lib_dir),
        join(TEMP_PATH, "WasmEdge-tensorflow-lite-deps", CONST_lib_dir),
        env_file_path=CONST_env_path,
        remove_finished=True,
    )

    copytree(join(TEMP_PATH, "WasmEdge-tensorflow-tools"), args.path)
    copytree(join(TEMP_PATH, "WasmEdge-tensorflow-lite-deps"), args.path)
    copytree(join(TEMP_PATH, "WasmEdge-tensorflow-deps"), args.path)

    all_files = run_shell_command("ls -R {0}".format(TEMP_PATH))

    # make symlinks
    other_lib_dir = None
    if CONST_lib_dir == "lib":
        other_lib_dir = "lib64"
    else:
        other_lib_dir = "lib"
    for lib_dir in [CONST_lib_dir, other_lib_dir]:
        if not isdir(join(args.path, lib_dir)):
            continue
        for file in listdir(join(args.path, lib_dir)):
            if CONST_lib_ext not in file:
                # ignore files that are not libraries
                continue
            if file not in all_files:
                # ignore files that are not downloaded by this script
                continue
            if "tensorflow" not in file:
                continue
            # check if it contains any digits
            if not any(i.isdigit() for i in file):
                continue
            if compat.platform == "Linux":
                name, version = file.split(CONST_lib_ext, 1)
                if version != "":
                    no_v_name = name + CONST_lib_ext
                    single_v_name = name + CONST_lib_ext + "." + version.split(".")[1]
                    dual_v_name = (
                        name
                        + CONST_lib_ext
                        + "."
                        + version.split(".")[1]
                        + "."
                        + version.split(".")[2]
                    )
                    file_path = join(args.path, lib_dir, file)
                    single_v_file_path = join(args.path, lib_dir, single_v_name)
                    dual_v_file_path = join(args.path, lib_dir, dual_v_name)
                    no_v_file_path = join(args.path, lib_dir, no_v_name)
                    try:
                        symlink(file_path, single_v_file_path)
                        symlink(file_path, dual_v_file_path)
                        symlink(file_path, no_v_file_path)
                    except Exception as e:
                        logging.debug(e)
            elif compat.platform == "Darwin":
                name, version = file.split(CONST_lib_ext, 1)[0].split(".", 1)
                if version != "":
                    no_v_name = name + CONST_lib_ext
                    single_v_name = name + "." + version.split(".")[0] + CONST_lib_ext
                    dual_v_name = (
                        name
                        + "."
                        + version.split(".")[0]
                        + "."
                        + version.split(".")[1]
                        + CONST_lib_ext
                    )
                    file_path = join(args.path, lib_dir, file)
                    single_v_file_path = join(args.path, lib_dir, single_v_name)
                    dual_v_file_path = join(args.path, lib_dir, dual_v_name)
                    no_v_file_path = join(args.path, lib_dir, no_v_name)
                    try:
                        symlink(file_path, single_v_file_path)
                        symlink(file_path, dual_v_file_path)
                        symlink(file_path, no_v_file_path)
                    except Exception as e:
                        logging.debug(e)
            else:
                reraise(Exception("Not implemented for {0}".format(compat.platform)))
            with opened_w_error(CONST_env_path, "a") as env_file:
                if env_file is not None:
                    env_file.write("#" + single_v_file_path + "\n")
                    logging.debug("Appending:%s", single_v_file_path)
                    env_file.write("#" + dual_v_file_path + "\n")
                    logging.debug("Appending:%s", dual_v_file_path)
                    env_file.write("#" + no_v_file_path + "\n")
                    logging.debug("Appending:%s", no_v_file_path)
                else:
                    logging.error("Not able to append installed files to env file")

    for main_dir in ["WasmEdge-tensorflow", "WasmEdge-tensorflow-lite"]:
        for directory_file in listdir(join(TEMP_PATH, main_dir)):
            if isdir(directory_file):
                wasmedge_tf_folder = join(TEMP_PATH, main_dir, directory_file)
                for _file in listdir(wasmedge_tf_folder):
                    if (
                        _file == "wasmedge"
                        and isdir(join(wasmedge_tf_folder, _file))
                        and args.path == abspath(PATH)
                    ):
                        copytree(
                            join(wasmedge_tf_folder, _file),
                            join(args.path, "include"),
                        )
                    elif CONST_lib_ext in _file:
                        if isdir(join(args.path, CONST_lib_dir)):
                            shutil.move(
                                join(wasmedge_tf_folder, _file),
                                join(args.path, CONST_lib_dir, _file),
                            )
                        else:
                            shutil.move(
                                join(wasmedge_tf_folder, _file),
                                join(args.path, "lib", _file),
                            )
                    elif isdir(join(wasmedge_tf_folder, _file)):
                        copytree(
                            join(wasmedge_tf_folder, _file),
                            join(args.path, _file),
                        )
                    else:
                        shutil.move(
                            join(wasmedge_tf_folder, _file),
                            join(args.path, "bin", _file),
                        )

    # Check if wasmedge binary works
    wasmedge_tf_output = run_shell_command(
        ". {0}/env &&{0}/bin/wasmedge-tensorflow --version".format(args.path)
    )

    if args.tf_version in wasmedge_tf_output:
        print("WasmEdge Successfully installed")
    else:
        logging.critical(
            "WasmEdge Tensorflow installation incorrect: {0}".format(wasmedge_tf_output)
        )

    # Check if wasmedge binary works
    wasmedge_tf_lite_output = run_shell_command(
        ". {0}/env && {0}/bin/wasmedge-tensorflow-lite --version".format(args.path)
    )

    if args.tf_version in wasmedge_tf_lite_output:
        print("WasmEdge Tensorflow Lite Successfully installed")
    else:
        logging.critical(
            "WasmEdge Tensorflow installation incorrect: {0}".format(
                wasmedge_tf_lite_output
            )
        )

    return 0


def set_consts(args, compat):
    global CONST_release_pkg, CONST_ipkg, CONST_lib_ext, CONST_urls, CONST_lib_dir, CONST_env_path
    CONST_release_pkg = compat.release_package
    CONST_ipkg = compat.install_package_name
    CONST_lib_ext = compat.lib_extension

    if compat.machine in [
        "x86_64",
        "arm64",
        "amd",
        "aarch64",
    ]:
        CONST_lib_dir = "lib64"

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
        TENSORFLOW_DEPS: "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/{0}/WasmEdge-tensorflow-deps-TF-{0}-{1}".format(
            args.tf_deps_version, CONST_release_pkg
        ),
        TENSORFLOW_LITE_DEPS: "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/{0}/WasmEdge-tensorflow-deps-TFLite-{0}-{1}".format(
            args.tf_deps_version, CONST_release_pkg
        ),
        TENSORFLOW: "https://github.com/second-state/WasmEdge-tensorflow/releases/download/{0}/WasmEdge-tensorflow-{0}-{1}".format(
            args.tf_version, CONST_release_pkg
        ),
        TENSORFLOW_LITE: "https://github.com/second-state/WasmEdge-tensorflow/releases/download/{0}/WasmEdge-tensorflowlite-{0}-{1}".format(
            args.tf_version, CONST_release_pkg
        ),
        TENSORFLOW_TOOLS: "https://github.com/second-state/WasmEdge-tensorflow-tools/releases/download/{0}/WasmEdge-tensorflow-tools-{0}-{1}".format(
            args.tf_tools_version, CONST_release_pkg
        ),
    }


def run_shell_command(cmd):
    try:
        output = subprocess.check_output([cmd], shell=True)
        return output.decode("utf8").strip()
    except subprocess.CalledProcessError as e:
        if "Cannot detect installation path" in str(e.output):
            logging.warning("Uninstaller did not find previous installation")
        else:
            print("Exception on process, rc=", e.returncode, "output=", e.output, e.cmd)

    return ""


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
        elif self.extensions is not None and len(self.extensions) > 0:
            if not (
                set(self.extensions)
                <= set(SUPPORTED_EXTENSIONS[self.platform + self.machine])
            ):
                reraise(
                    Exception(
                        "Extensions not supported: {0}. Supported extensions: {1}".format(
                            self.extensions,
                            SUPPORTED_EXTENSIONS[self.platform + self.machine],
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
    global CONST_env_path, CONST_release_pkg, CONST_ipkg, CONST_shell_config, CONST_shell_profile, CONST_lib_dir

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
        logging.debug("CONST_urls: %s", CONST_urls)
        logging.debug("CONST_lib_dir: %s", CONST_lib_dir)

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

        if args.path == abspath(PATH):
            # perform actions if default path
            for dir in listdir(args.path):
                path = join(args.path, dir)
                if not isdir(path):
                    continue
                for subdir in listdir(path):
                    sub_folder = join(path, subdir)
                    if isdir(sub_folder):
                        if any("Plugin" in s for s in listdir(sub_folder)):
                            # Handle plugins
                            copytree(sub_folder, join(args.path, "plugin"), True)
                        else:
                            copytree(sub_folder, path, True)
                        shutil.rmtree(sub_folder)
            if isdir(join(args.path, "lib64")):
                try:
                    symlink(
                        join(args.path, "lib64"),
                        join(args.path, "lib"),
                        target_is_directory=True,
                    )
                except Exception as e:
                    logging.debug(e)
                with opened_w_error(CONST_env_path, "a") as env_file:
                    if env_file is not None:
                        env_file.write("#" + join(args.path, "lib") + "\n")
                        logging.debug("Appending:%s", join(args.path, "lib"))
                    else:
                        logging.error("Not able to write installed files to env")

        # Check if wasmedge binary works
        wasmedge_output = run_shell_command(
            "{0}/bin/wasmedge --version".format(args.path)
        )

        if args.version in wasmedge_output:
            print("WasmEdge Successfully installed")
        else:
            logging.critical(
                "WasmEdge installation incorrect: {0}".format(wasmedge_output)
            )

        if IMAGE in args.extensions or "all" in args.extensions:
            if install_image_extension(args, compat) != 0:
                logging.error("Error in installing image extensions")
            else:
                print("Image extension installed")

        if TENSORFLOW in args.extensions or "all" in args.extensions:
            if install_tensorflow_extension(args, compat) != 0:
                logging.error("Error in installing tensorflow extensions")
            else:
                print("Tensorflow extension installed")

        # Cleanup
        shutil.rmtree(TEMP_PATH)

        print("Run:\nsource {0}".format(CONST_shell_config))
    else:
        reraise(Exception("Incompatible with your machine\n{0}".format(compat)))


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
        nargs="*",
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
