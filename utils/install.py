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
import shutil
import sys
import argparse
from os.path import expanduser, join, dirname, abspath, exists, islink, lexists, isdir
from os import (
    getenv,
    geteuid,
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

download_url = None

# Define version specific things
if sys.version_info[0] == 3:
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
                if fname.endswith("wasmedge") and not fname.endswith("bin/wasmedge"):
                    continue

                # replace wasmedge folder name with include
                if is_default_path(args):
                    fname = fname.replace("/lib64/", "/" + CONST_lib_dir + "/")
                if fname.endswith("/lib64"):
                    fname = fname[:-5] + "lib"
                if fname.startswith("/usr") and "lib64" in fname:
                    fname = fname.replace("lib64", "lib", 1)
                if "Plugin" in fname:
                    if is_default_path(args):
                        fname = fname.replace(
                            join(ipath, CONST_lib_dir, "wasmedge/"), ""
                        )

                        fname = join(ipath, "plugin", fname)
                    else:
                        fname = join(ipath, CONST_lib_dir, "wasmedge", fname)
                else:
                    if ipath not in fname:
                        fname = join(ipath, fname)
                # replace GNUSparseFile.0 with nothing
                fname = fname.replace("/GNUSparseFile.0", "")
                # Don't append system directories
                if (not is_default_path(args)) and isdir(fname):
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

    def __repr__(self):
        return "VersionString:" + self.version

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
    "Linux" + "x86_64": VersionString("0.9.0"),
    "Linux" + "amd64": VersionString("0.9.0"),
    "Linux" + "arm64": VersionString("0.9.0"),
    "Linux" + "armv8": VersionString("0.9.0"),
    "Linux" + "aarch64": VersionString("0.9.0"),
    "Darwin" + "x86_64": VersionString("0.9.0"),
    "Darwin" + "arm64": VersionString("0.9.0"),
    "Darwin" + "arm": VersionString("0.9.0"),
}

WASMEDGE = "WasmEdge"
WASMEDGE_UNINSTALLER = "WasmEdge_Uninstaller"
TENSORFLOW = "tensorflow"
TENSORFLOW_LITE = "tensorflow_lite"
TENSORFLOW_DEPS = "tensorflow_deps"
TENSORFLOW_LITE_DEPS = "tensorflow_lite_deps"
TENSORFLOW_TOOLS = "tensorflow_tools"
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

SUPPORTED_EXTENSIONS_VERSION = {
    "Linux" + "x86_64" + TENSORFLOW: VersionString("0.9.0"),
    "Linux" + "x86_64" + IMAGE: VersionString("0.9.0"),
    "Linux" + "amd64" + TENSORFLOW: VersionString("0.9.0"),
    "Linux" + "amd64" + IMAGE: VersionString("0.9.0"),
    "Linux" + "arm64" + TENSORFLOW: VersionString("0.9.0"),
    "Linux" + "arm64" + IMAGE: VersionString("0.9.0"),
    "Linux" + "armv8" + TENSORFLOW: VersionString("0.9.0"),
    "Linux" + "armv8" + IMAGE: VersionString("0.9.0"),
    "Linux" + "aarch64" + TENSORFLOW: VersionString("0.9.1-beta.1"),
    "Linux" + "aarch64" + IMAGE: VersionString("0.9.1-beta.1"),
    "Darwin" + "x86_64" + TENSORFLOW: VersionString("0.10.0-alpha.1"),
    "Darwin" + "x86_64" + IMAGE: VersionString("0.10.0-alpha.1"),
    "Darwin" + "arm64" + TENSORFLOW: VersionString("0.10.0-alpha.1"),
    "Darwin" + "arm" + TENSORFLOW: VersionString("0.10.0-alpha.1"),
}

WASI_NN_OPENVINO = "wasi_nn-openvino"
WASI_CRYPTO = "wasi_crypto"
WASI_NN_PYTORCH = "wasi_nn-pytorch"
WASI_NN_TENSORFLOW_LITE = "wasi_nn-tensorflowlite"
WASMEDGE_HTTPSREQ = "wasmedge_httpsreq"
PLUGINS_AVAILABLE = [
    WASI_NN_OPENVINO,
    WASI_CRYPTO,
    WASI_NN_PYTORCH,
    WASI_NN_TENSORFLOW_LITE,
    WASMEDGE_HTTPSREQ,
]

SUPPORTTED_PLUGINS = {
    "ubuntu20.04" + "x86_64" + WASI_CRYPTO: VersionString("0.10.1-rc.1"),
    "manylinux2014" + "x86_64" + WASI_CRYPTO: VersionString("0.10.1-rc.1"),
    "manylinux2014" + "aarch64" + WASI_CRYPTO: VersionString("0.10.1-rc.1"),
    "manylinux2014" + "arm64" + WASI_CRYPTO: VersionString("0.10.1-rc.1"),
    "ubuntu20.04" + "x86_64" + WASI_NN_OPENVINO: VersionString("0.10.1-alpha.1"),
    "ubuntu20.04" + "x86_64" + WASI_NN_PYTORCH: VersionString("0.11.1-alpha.1"),
    "manylinux2014" + "x86_64" + WASI_NN_PYTORCH: VersionString("0.11.2-alpha.1"),
    "manylinux2014"
    + "x86_64"
    + WASI_NN_TENSORFLOW_LITE: VersionString("0.11.2-alpha.1"),
    "manylinux2014"
    + "aarch64"
    + WASI_NN_TENSORFLOW_LITE: VersionString("0.11.2-alpha.1"),
    "ubuntu20.04" + "x86_64" + WASI_NN_TENSORFLOW_LITE: VersionString("0.11.2-rc.1"),
    "ubuntu20.04" + "x86_64" + WASMEDGE_HTTPSREQ: VersionString("0.11.1"),
    "manylinux2014" + "x86_64" + WASMEDGE_HTTPSREQ: VersionString("0.11.1"),
    "manylinux2014" + "aarch64" + WASMEDGE_HTTPSREQ: VersionString("0.11.1"),
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
if [ -z ${{WASMEDGE_LIB_DIR+x}} ]; then
    export WASMEDGE_LIB_DIR="{0}/{6}"
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
    )

    try:
        mkdir(args.path)
        if is_default_path(args):
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


def shell_configure(args, compat):

    global CONST_shell_profile, CONST_shell_config

    source_string = '\n. "{0}"\n'.format(join(args.path, "env"))

    if ("bash" in SHELL) or ("zsh" in SHELL):

        CONST_shell_config = join(HOME, "." + SHELL + "rc")

        if "zsh" in SHELL:
            CONST_shell_profile = join(HOME, "." + "zprofile")
        else:
            CONST_shell_profile = join(HOME, "." + SHELL + "_profile")

        # On Darwin: Create shell config only if shell_profile does not exist
        # On Linux: Create shell config anyway
        if not exists(CONST_shell_config) and compat.platform != "Darwin":
            open(CONST_shell_config, "a").close()

        write_shell = False
        if compat.platform != "Darwin":
            with opened_w_error(CONST_shell_config, "r") as shell_config:
                if shell_config is not None:
                    if source_string not in shell_config.read():
                        write_shell = True

        # On Darwin: Append to shell config only if shell_profile does not exist
        # On Linux: Append to shell config anyway
        if write_shell and compat.platform != "Darwin":
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


def fix_gnu_sparse(args):
    # Fix GNUSparseFile.0 folder in macOS if exists
    global CONST_lib_ext, CONST_lib_dir

    for dir in listdir(args.path):
        if not isdir(join(args.path, dir)):
            continue
        if "GNUSparseFile" in dir:
            for file in listdir(join(args.path, dir)):
                if file.endswith(CONST_lib_ext):
                    if isdir(join(args.path, CONST_lib_dir)):
                        shutil.move(
                            join(args.path, dir, file), join(args.path, CONST_lib_dir)
                        )
                    else:
                        logging.error(
                            "%s directory not found", join(args.path, CONST_lib_dir)
                        )
                        try:
                            mkdir(join(args.path, CONST_lib_dir))
                            shutil.move(
                                join(args.path, dir, file),
                                join(args.path, CONST_lib_dir),
                            )
                        except:
                            pass
                elif (
                    file.endswith(".h")
                    or file.endswith(".hpp")
                    or file.endswith(".inc")
                ):
                    shutil.move(join(args.path, dir, file), join(args.path, "include"))
                else:
                    shutil.move(join(args.path, dir, file), join(args.path, "bin"))
        for sub_dir in listdir(join(args.path, dir)):
            if not isdir(join(args.path, dir, sub_dir)):
                continue
            if "GNUSparseFile" in sub_dir:
                for file in listdir(join(args.path, dir, sub_dir)):
                    shutil.move(
                        join(args.path, dir, sub_dir, file), join(args.path, dir)
                    )
                if len(listdir(join(args.path, dir, sub_dir))) == 0:
                    shutil.rmtree(join(args.path, dir, sub_dir))


def ldconfig(args, compat):
    if geteuid() == 0:
        # Only run ldconfig or update_dyld_shared_cache when user is root/sudoer
        if compat.platform == "Linux":
            cmd = "ldconfig {0}".format(join(args.path, CONST_lib_dir))
            output = run_shell_command(cmd)
            logging.debug("%s: %s", cmd, output)
        elif compat.platform == "Darwin":
            cmd = "update_dyld_shared_cache {0}".format(join(args.path, CONST_lib_dir))
            output = run_shell_command(cmd)
            logging.debug("%s: %s", cmd, output)
        else:
            logging.warning("Help adding ldconfig for your platform")
    else:
        logging.debug("Not root or sudoer, skip ldconfig")


def is_default_path(args):
    global PATH
    return args.path == abspath(PATH) or args.path[:4] != "/usr"


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

    # Extract archive
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
            if isdir(join(wasmedge_image_temp_dir, file)) and "wasmedge" == file:
                copytree(
                    join(wasmedge_image_temp_dir, file),
                    join(args.path, "include", "wasmedge"),
                )
            elif CONST_lib_ext in file:
                if isdir(join(args.path, CONST_lib_dir)):
                    shutil.move(
                        join(wasmedge_image_temp_dir, file),
                        join(args.path, CONST_lib_dir, file),
                    )
                else:
                    logging.error(
                        "%s directory not found", join(args.path, CONST_lib_dir)
                    )
                    try:
                        mkdir(join(args.path, CONST_lib_dir))
                        shutil.move(
                            join(wasmedge_image_temp_dir, file),
                            join(args.path, "lib", file),
                        )
                    except:
                        pass
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

    fix_gnu_sparse(args)

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

    download_tf = True
    download_tf_lite = True

    if compat.machine == "aarch64":
        download_tf = False

    if download_tf:
        tf_pkg = "WasmEdge-tensorflow-" + args.tf_version + "-" + CONST_release_pkg
        tf_deps_pkg = (
            "WasmEdge-tensorflow-deps-TF-"
            + args.tf_deps_version
            + "-"
            + CONST_release_pkg
        )

        print("Downloading tensorflow extension")
        download_url(CONST_urls[TENSORFLOW], join(TEMP_PATH, tf_pkg), show_progress)

        print("Downloading tensorflow-deps")
        download_url(
            CONST_urls[TENSORFLOW_DEPS], join(TEMP_PATH, tf_deps_pkg), show_progress
        )

        # Extract archive
        extract_archive(
            join(TEMP_PATH, tf_pkg),
            args.path,
            join(TEMP_PATH, "WasmEdge-tensorflow"),
            env_file_path=CONST_env_path,
            remove_finished=True,
        )

        # Extract archive
        extract_archive(
            join(TEMP_PATH, tf_deps_pkg),
            join(args.path, CONST_lib_dir),
            join(TEMP_PATH, "WasmEdge-tensorflow-deps", CONST_lib_dir),
            env_file_path=CONST_env_path,
            remove_finished=True,
        )

        copytree(join(TEMP_PATH, "WasmEdge-tensorflow"), args.path)
        copytree(join(TEMP_PATH, "WasmEdge-tensorflow-deps"), args.path)

    if download_tf_lite:
        tf_lite_pkg = (
            "WasmEdge-tensorflowlite-" + args.tf_version + "-" + CONST_release_pkg
        )
        tf_deps_lite_pkg = (
            "WasmEdge-tensorflow-deps-TFLite-"
            + args.tf_deps_version
            + "-"
            + CONST_release_pkg
        )

        print("Downloading tensorflow-lite extension")
        download_url(
            CONST_urls[TENSORFLOW_LITE], join(TEMP_PATH, tf_lite_pkg), show_progress
        )

        print("Downloading tensorflow-lite-deps")
        download_url(
            CONST_urls[TENSORFLOW_LITE_DEPS],
            join(TEMP_PATH, tf_deps_lite_pkg),
            show_progress,
        )

        # Extract archive
        extract_archive(
            join(TEMP_PATH, tf_lite_pkg),
            args.path,
            join(TEMP_PATH, "WasmEdge-tensorflow-lite"),
            env_file_path=CONST_env_path,
            remove_finished=True,
        )

        # Extract archive
        extract_archive(
            join(TEMP_PATH, tf_deps_lite_pkg),
            join(args.path, CONST_lib_dir),
            join(TEMP_PATH, "WasmEdge-tensorflow-lite-deps", CONST_lib_dir),
            env_file_path=CONST_env_path,
            remove_finished=True,
        )

        copytree(join(TEMP_PATH, "WasmEdge-tensorflow-lite"), args.path)
        copytree(join(TEMP_PATH, "WasmEdge-tensorflow-lite-deps"), args.path)

    tf_tools_pkg = (
        "WasmEdge-tensorflow-tools-" + args.tf_tools_version + "-" + CONST_release_pkg
    )

    print("Downloading tensorflow-tools extension")
    download_url(
        CONST_urls[TENSORFLOW_TOOLS], join(TEMP_PATH, tf_tools_pkg), show_progress
    )

    # Extract archive
    extract_archive(
        join(TEMP_PATH, tf_tools_pkg),
        join(args.path, "bin"),
        join(TEMP_PATH, "WasmEdge-tensorflow-tools", "bin"),
        env_file_path=CONST_env_path,
        remove_finished=True,
    )

    copytree(join(TEMP_PATH, "WasmEdge-tensorflow-tools"), args.path)

    fix_gnu_sparse(args)

    all_files = run_shell_command("ls -R {0}".format(TEMP_PATH))

    if not isdir(join(args.path, CONST_lib_dir)):
        logging.error("Strange: No %s directory found", CONST_lib_dir)

    for file in listdir(join(args.path, CONST_lib_dir)):
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
            if version[0] == ".":
                version = version[1:]
            if version != "" and version.count(".") >= 2:
                no_v_name = name + CONST_lib_ext
                single_v_name = name + CONST_lib_ext + "." + version.split(".")[0]
                dual_v_name = (
                    name
                    + CONST_lib_ext
                    + "."
                    + version.split(".")[0]
                    + "."
                    + version.split(".")[1]
                )
                file_path = join(args.path, CONST_lib_dir, file)
                single_v_file_path = join(args.path, CONST_lib_dir, single_v_name)
                dual_v_file_path = join(args.path, CONST_lib_dir, dual_v_name)
                no_v_file_path = join(args.path, CONST_lib_dir, no_v_name)
                try:
                    symlink(file_path, single_v_file_path)
                    symlink(file_path, dual_v_file_path)
                    symlink(file_path, no_v_file_path)
                except Exception as e:
                    logging.debug(e)
            else:
                continue
        elif compat.platform == "Darwin":
            name, version = file.split(CONST_lib_ext, 1)[0].split(".", 1)
            if version != "" and version.count(".") >= 2:
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
                file_path = join(args.path, CONST_lib_dir, file)
                single_v_file_path = join(args.path, CONST_lib_dir, single_v_name)
                dual_v_file_path = join(args.path, CONST_lib_dir, dual_v_name)
                no_v_file_path = join(args.path, CONST_lib_dir, no_v_name)
                try:
                    symlink(file_path, single_v_file_path)
                    symlink(file_path, dual_v_file_path)
                    symlink(file_path, no_v_file_path)
                except Exception as e:
                    logging.debug(e)
            else:
                continue
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
        if not isdir(join(TEMP_PATH, main_dir)):
            continue
        for directory_file in listdir(join(TEMP_PATH, main_dir)):
            if isdir(directory_file):
                wasmedge_tf_folder = join(TEMP_PATH, main_dir, directory_file)
                for _file in listdir(wasmedge_tf_folder):
                    if (
                        _file == "wasmedge"
                        and isdir(join(wasmedge_tf_folder, _file))
                        and is_default_path(args)
                    ):
                        copytree(
                            join(wasmedge_tf_folder, _file),
                            join(args.path, "include", "wasmedge"),
                        )
                    elif CONST_lib_ext in _file:
                        if isdir(join(args.path, CONST_lib_dir)):
                            shutil.move(
                                join(wasmedge_tf_folder, _file),
                                join(args.path, CONST_lib_dir, _file),
                            )
                        else:
                            logging.error(
                                "%s is not a directory", join(args.path, CONST_lib_dir)
                            )
                            try:
                                mkdir(join(args.path, CONST_lib_dir))
                                shutil.move(
                                    join(wasmedge_tf_folder, _file),
                                    join(args.path, CONST_lib_dir, _file),
                                )
                            except:
                                pass

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

    if download_tf:
        # Check if wasmedge binary works
        wasmedge_tf_output = run_shell_command(
            ". {0}/env &&{0}/bin/wasmedge-tensorflow --version".format(args.path)
        )

        if args.tf_version in wasmedge_tf_output:
            print("WasmEdge Successfully installed")
        else:
            logging.critical(
                "WasmEdge Tensorflow installation incorrect: {0}".format(
                    wasmedge_tf_output
                )
            )

    if download_tf_lite:
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


def install_plugins(args, compat):
    global CONST_lib_dir
    url_root = "https://github.com/WasmEdge/WasmEdge/releases/download/"
    url_root += "$VERSION$/WasmEdge-plugin-$PLUGIN_NAME$-$VERSION$-$DIST$_$ARCH$.tar.gz"

    if len(args.plugins) >= 1:
        for plugin_name in args.plugins:
            plugin_version_supplied = None
            if plugin_name.find(":") != -1:
                plugin_name, plugin_version_supplied = plugin_name.split(":")

            if plugin_name not in PLUGINS_AVAILABLE:
                logging.error(
                    "%s plugin not found, available names - %s",
                    plugin_name,
                    PLUGINS_AVAILABLE,
                )
                continue

            if compat.dist + compat.machine + plugin_name not in SUPPORTTED_PLUGINS:
                logging.error(
                    "Plugin not compatible: %s",
                    compat.dist + compat.machine + plugin_name,
                )
                logging.debug("Supported: %s", SUPPORTTED_PLUGINS)
                continue
            else:
                if plugin_version_supplied is None:
                    plugin_version_supplied = args.version
                elif (
                    SUPPORTTED_PLUGINS[
                        compat.dist + compat.machine + plugin_name
                    ].compare(plugin_version_supplied)
                    > 0
                ):
                    logging.error(
                        "Plugin not compatible: %s %s",
                        plugin_name,
                        plugin_version_supplied,
                    )
                    continue

                plugin_url = (
                    url_root.replace("$PLUGIN_NAME$", plugin_name)
                    .replace("$VERSION$", plugin_version_supplied)
                    .replace("$DIST$", compat.dist)
                    .replace("$ARCH$", compat.machine)
                )
                logging.debug("Plugin URL: %s", plugin_url)

                print("Downloading Plugin: " + plugin_name)
                download_url(
                    plugin_url,
                    join(TEMP_PATH, "Plugin" + plugin_name) + ".tar.gz",
                    show_progress,
                )
                extract_archive(
                    join(TEMP_PATH, "Plugin" + plugin_name + ".tar.gz"),
                    join(args.path),
                    join(TEMP_PATH, "Plugins"),
                    env_file_path=CONST_env_path,
                    remove_finished=True,
                )

        if isdir(join(TEMP_PATH, "Plugins")):
            if is_default_path(args):
                copytree(join(TEMP_PATH, "Plugins"), join(args.path, "plugin"))
            else:
                copytree(
                    join(TEMP_PATH, "Plugins"),
                    join(args.path, CONST_lib_dir, "wasmedge"),
                )


def set_consts(args, compat):
    global CONST_release_pkg, CONST_ipkg, CONST_lib_ext, CONST_urls, CONST_lib_dir, CONST_env_path
    CONST_release_pkg = compat.release_package
    CONST_ipkg = compat.install_package_name
    CONST_lib_ext = compat.lib_extension

    CONST_urls = {
        WASMEDGE: "https://github.com/WasmEdge/WasmEdge/releases/download/{0}/WasmEdge-{0}-{1}".format(
            args.version, compat.release_package_wasmedge
        ),
        WASMEDGE_UNINSTALLER: "https://raw.githubusercontent.com/WasmEdge/WasmEdge/{0}/utils/uninstall.sh".format(
            args.uninstall_script_tag
        ),
        IMAGE: "https://github.com/second-state/WasmEdge-image/releases/download/{0}/WasmEdge-image-{0}-{1}".format(
            args.image_version, CONST_release_pkg
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
        platform_=platform.system(),
        machine=platform.machine(),
        dist_=None,
        version=None,
        extensions=None,
    ):
        self.platform = platform_  # Linux, Darwin
        self.machine = machine  # x86_64, arm
        self.version = VersionString(version)
        self.extensions = extensions
        self.release_package = None
        self.install_package_name = None
        self.lib_extension = None
        self.ld_library_path = None
        self.dist = dist_
        self.release_package_wasmedge = None

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

            self.release_package_wasmedge = self.release_package

            if self.dist is None:
                if sys.version_info[0] == 2:
                    if (
                        "Ubuntu" in platform.dist() and "20.04" in platform.dist()
                    ) or "Ubuntu 20.04" in run_shell_command(
                        "lsb_release -d | awk -F'\t' '{print $2}'"
                    ):
                        self.dist = "ubuntu20.04"
                    else:
                        self.dist = "manylinux2014"
                elif sys.version_info[0] == 3:
                    __lsb_rel = run_shell_command(
                        "cat /etc/lsb-release | grep RELEASE"
                    )[-5:]
                    if "20.04" == __lsb_rel or "Ubuntu 20.04" in run_shell_command(
                        "lsb_release -d | awk -F'\t' '{print $2}'"
                    ):
                        self.dist = "ubuntu20.04"
                    else:
                        self.dist = "manylinux2014"

            # Below version 0.11.1 different distributions for wasmedge binary do not exist
            if self.version.compare("0.11.1") != -1:
                if self.machine in ["arm64", "armv8", "aarch64"]:
                    self.release_package_wasmedge = self.dist + "_aarch64.tar.gz"
                elif self.machine in ["x86_64", "amd64"]:
                    self.release_package_wasmedge = self.dist + "_x86_64.tar.gz"
                else:
                    reraise(Exception("Unsupported arch: {0}".format(self.machine)))

        elif self.platform == "Darwin":
            self.ld_library_path = "DYLD_LIBRARY_PATH"
            self.install_package_name = "WasmEdge-{0}-Darwin".format(self.version)
            self.release_package = "darwin_{0}.tar.gz".format(self.machine)
            self.release_package_wasmedge = self.release_package
            self.lib_extension = ".dylib"
            if self.dist is None:
                self.dist = "darwin"

    def __str__(self):
        return (
            "Platform:{0}\nMachine:{1}\nVersion:{2}\nExtensions:{3}\nDist:{4}\n".format(
                self.platform, self.machine, self.version, self.extensions, self.dist
            )
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
        if self.machine not in SUPPORTED_PLATFORM_MACHINE[self.platform]:
            reraise(Exception("Unsupported machine: {0}".format(self.machine)))
        if self.extensions is not None and len(self.extensions) > 0:
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
        if (
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

        if not get_remote_version_availability(
            "WasmEdge/WasmEdge", self.version.version
        ):
            reraise(
                Exception(
                    "Version {0} does not exist in remote repository of WasmEdge".format(
                        self.version.version
                    )
                )
            )
        return True

    def prefix(self):
        return self.platform + self.machine


def main(args):
    global CONST_env_path, CONST_release_pkg, CONST_ipkg, CONST_shell_config, CONST_shell_profile, CONST_lib_dir

    compat = Compat(
        version=args.version,
        extensions=args.extensions,
        platform_=args.platform,
        machine=args.machine,
        dist_=args.dist,
    )

    logging.debug("Compat object: %s", compat)
    logging.debug("Temp path: %s", TEMP_PATH)
    logging.debug("CLI Args:")
    logging.debug(args)

    if len(args.plugins) >= 1:
        logging.warning("Experimental Option Selected: plugins")
        logging.warning("plugins option may change later")

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

        if shell_configure(args, compat) != 0:
            logging.error("Error in configuring shell")

        logging.debug("CONST_shell_profile: %s", CONST_shell_profile)
        logging.debug("CONST_shell_config: %s", CONST_shell_config)

        print("Downloading WasmEdge")

        # Download WasmEdge
        download_url(
            CONST_urls[WASMEDGE], join(TEMP_PATH, CONST_release_pkg), show_progress
        )

        # Extract archive
        extract_archive(
            join(TEMP_PATH, CONST_release_pkg),
            args.path,
            join(TEMP_PATH),
            env_file_path=CONST_env_path,
            remove_finished=True,
        )

        print("Installing WasmEdge")
        # Copy the tree
        for sub_dir in listdir(join(TEMP_PATH, CONST_ipkg)):
            if sub_dir == "lib64":
                copytree(join(TEMP_PATH, CONST_ipkg, sub_dir), join(args.path, "lib"))
            else:
                copytree(join(TEMP_PATH, CONST_ipkg, sub_dir), join(args.path, sub_dir))

        if is_default_path(args):
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
                            shutil.rmtree(sub_folder)

        # Check if wasmedge binary works
        wasmedge_output = run_shell_command(
            ". {0}/env && {0}/bin/wasmedge --version".format(args.path)
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

        install_plugins(args, compat)

        ldconfig(args, compat)

        # Cleanup
        shutil.rmtree(TEMP_PATH)

        if compat.platform != "Darwin":
            print("Run:\nsource {0}".format(CONST_shell_config))
        else:
            print("Run:\nsource {0}".format(CONST_shell_profile))
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
        "--plugins",
        dest="plugins",
        required=False,
        default=[],
        nargs="*",
        help="(experimental option)Supported Plugins. Example\n"
        + "--plugins wasi_crypto:0.11.0\n"
        + "--plugins wasi_crypto",
    )
    parser.add_argument(
        "--tf-version",
        dest="tf_version",
        required=False,
        default=None,
        help="Tensorflow and tensorflow lite version",
    )
    parser.add_argument(
        "--tf-deps-version",
        dest="tf_deps_version",
        required=False,
        default=None,
        help="Tensorflow and tensorflow lite deps version",
    )
    parser.add_argument(
        "--tf-tools-version",
        dest="tf_tools_version",
        required=False,
        default=None,
        help="Tensorflow and tensorflow lite tools version",
    )
    parser.add_argument(
        "--image-version",
        dest="image_version",
        required=False,
        default=None,
        help="Image extension version",
    )
    parser.add_argument(
        "--platform",
        "--os",
        dest="platform",
        required=False,
        default=platform.system(),
        choices=["Linux", "Darwin"],
        type=lambda s: s.title(),
        help="Platform ex- Linux, Darwin, Windows",
    )
    parser.add_argument(
        "--machine",
        "--arch",
        dest="machine",
        required=False,
        default=platform.machine(),
        choices=["x86_64", "aarch64", "arm", "arm64"],
        type=lambda s: s.lower(),
        help="Machine ex- x86_64, aarch64",
    )
    parser.add_argument(
        "--dist",
        dest="dist",
        required=False,
        default=None,
        choices=["ubuntu20.04", "manylinux2014"],
        type=lambda s: s.lower(),
        help="Dist ex- ubuntu20.04,manylinux2014",
    )
    args = parser.parse_args()

    logging.basicConfig(format="%(levelname)-8s- %(message)s", level=args.loglevel)

    args.path = abspath(args.path)

    if args.tf_version is None:
        args.tf_version = args.version

    if args.tf_deps_version is None:
        args.tf_deps_version = args.version

    if args.tf_tools_version is None:
        args.tf_tools_version = args.version

    if args.image_version is None:
        args.image_version = args.version

    logging.debug("Python Version: %s", sys.version_info)
    main(args)
