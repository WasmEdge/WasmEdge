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
    import urllib.error

    def wrap_download_url(url, *args):
        try:
            return urllib.request.urlretrieve(url, *args)
        except urllib.error.HTTPError as e:
            logging.error("Download error from urllib: %s", e)
            logging.error("URL: %s", url)
            exit(1)

    download_url = wrap_download_url

    def reraise(tp, value=None, tb=None):
        if value is None:
            value = tp
        if value.__traceback__ is not tb:
            raise value.with_traceback(tb)
        raise value

else:
    exec("def reraise(tp, value=None, tb=None):\n    raise tp, value, tb\n")

    import urllib

    def wrap_download_url(url, *args):
        headers = ""
        try:
            _, headers = urllib.urlretrieve(url, *args)
        except:
            logging.error("Download error from urllib")
            logging.error("URL: %s", url)
            logging.debug("Header response: %s", headers)
            exit(1)

        if "text/plain" in str(headers) and not "uninstall" in args[0]:
            logging.error("Download error from urllib")
            logging.error("URL: %s", url)
            logging.debug("Header response: %s", headers)
            exit(1)

    download_url = wrap_download_url


def show_progress(block_num, block_size, total_size):
    downloaded = block_num * block_size
    downloaded_lim = min(1, downloaded / (total_size))

    print(
        end=(
            "\r|%-60s|" % ("=" * int(60 * downloaded_lim))
            + "%6.2f %%" % (downloaded_lim * 100)
        )
    )

    if downloaded < total_size:
        pass
    else:
        logging.info("Downloaded")


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

                if "._" in filename:
                    remove(join(to_path, filename))
                    continue
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

                if to_path.endswith("Plugins"):
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
            (
                int(x)
                if x.isdigit()
                else [
                    int(y) if y.isdigit() else y for y in re.findall("\d+|[a-zA-Z]+", x)
                ]
            )
            for x in re.split(separator, v)
        ]

    def compare(self, version2, separator=". |-", ignorecase=True):
        """
        # return 1 if self.version > version2
        # return 0 if self.version == version2
        # return -1 if self.version < version2
        # return False if not comparable
        """
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
    "Linux" + "x86_64": VersionString("0.13.0"),
    "Linux" + "amd64": VersionString("0.13.0"),
    "Linux" + "arm64": VersionString("0.13.0"),
    "Linux" + "armv8": VersionString("0.13.0"),
    "Linux" + "aarch64": VersionString("0.13.0"),
    "Darwin" + "x86_64": VersionString("0.13.0"),
    "Darwin" + "arm64": VersionString("0.13.0"),
    "Darwin" + "arm": VersionString("0.13.0"),
}

WASMEDGE = "WasmEdge"
WASMEDGE_UNINSTALLER = "WasmEdge_Uninstaller"
TENSORFLOW = "tensorflow"
TENSORFLOW_LITE = "tensorflow_lite"
TENSORFLOW_LITE_P = "tensorflowlite"
TENSORFLOW_DEPS = "tensorflow_deps"
TENSORFLOW_LITE_DEPS = "tensorflow_lite_deps"
IMAGE = "image"

WASI_NN_OPENVINO = "wasi_nn-openvino"
WASI_CRYPTO = "wasi_crypto"
WASI_NN_PYTORCH = "wasi_nn-pytorch"
WASI_NN_TENSORFLOW_LITE = "wasi_nn-tensorflowlite"
WASI_NN_GGML = "wasi_nn-ggml"
WASI_NN_GGML_CUDA = "wasi_nn-ggml-cuda"
WASI_NN_GGML_NOAVX = "wasi_nn-ggml-noavx"
WASI_LOGGING = "wasi_logging"
WASMEDGE_TENSORFLOW_PLUGIN = WASMEDGE.lower() + "_" + TENSORFLOW
WASMEDGE_TENSORFLOW_LITE_PLUGIN = WASMEDGE.lower() + "_" + TENSORFLOW_LITE_P
WASMEDGE_IMAGE_PLUGIN = WASMEDGE.lower() + "_" + IMAGE
WASMEDGE_RUSTLS = "wasmedge_rustls"
WASM_BPF = "wasm_bpf"

PLUGINS_AVAILABLE = [
    WASI_NN_OPENVINO,
    WASI_CRYPTO,
    WASI_NN_PYTORCH,
    WASI_NN_TENSORFLOW_LITE,
    WASI_NN_GGML,
    WASI_NN_GGML_CUDA,
    WASI_NN_GGML_NOAVX,
    WASI_LOGGING,
    WASMEDGE_TENSORFLOW_PLUGIN,
    WASMEDGE_TENSORFLOW_LITE_PLUGIN,
    WASMEDGE_IMAGE_PLUGIN,
    WASMEDGE_RUSTLS,
    WASM_BPF,
]

SUPPORTTED_PLUGINS = {
    "ubuntu20.04" + "x86_64" + WASI_CRYPTO: VersionString("0.13.0"),
    "manylinux2014" + "x86_64" + WASI_CRYPTO: VersionString("0.13.0"),
    "manylinux2014" + "aarch64" + WASI_CRYPTO: VersionString("0.13.0"),
    "manylinux2014" + "arm64" + WASI_CRYPTO: VersionString("0.13.0"),
    "ubuntu20.04" + "x86_64" + WASI_NN_OPENVINO: VersionString("0.13.0"),
    "ubuntu20.04" + "x86_64" + WASI_NN_PYTORCH: VersionString("0.13.0"),
    "ubuntu20.04" + "x86_64" + WASI_NN_GGML: VersionString("0.13.4"),
    "ubuntu20.04" + "aarch64" + WASI_NN_GGML: VersionString("0.13.5"),
    "ubuntu20.04" + "x86_64" + WASI_NN_GGML_NOAVX: VersionString("0.13.5"),
    "ubuntu20.04" + "x86_64" + WASI_NN_GGML_CUDA: VersionString("0.13.4"),
    "ubuntu20.04" + "aarch64" + WASI_NN_GGML_CUDA: VersionString("0.13.5"),
    "manylinux2014" + "x86_64" + WASI_NN_PYTORCH: VersionString("0.13.0"),
    "manylinux2014" + "x86_64" + WASI_NN_TENSORFLOW_LITE: VersionString("0.13.0"),
    "manylinux2014" + "x86_64" + WASI_NN_GGML: VersionString("0.13.4"),
    "manylinux2014" + "aarch64" + WASI_NN_TENSORFLOW_LITE: VersionString("0.13.0"),
    "manylinux2014" + "aarch64" + WASI_NN_GGML: VersionString("0.13.4"),
    "darwin" + "x86_64" + WASI_NN_GGML: VersionString("0.13.4"),
    "darwin" + "arm64" + WASI_NN_GGML: VersionString("0.13.4"),
    "ubuntu20.04" + "x86_64" + WASI_NN_TENSORFLOW_LITE: VersionString("0.13.0"),
    "darwin" + "x86_64" + WASMEDGE_TENSORFLOW_PLUGIN: VersionString("0.13.0"),
    "darwin" + "arm64" + WASMEDGE_TENSORFLOW_PLUGIN: VersionString("0.13.0"),
    "manylinux2014" + "x86_64" + WASMEDGE_TENSORFLOW_PLUGIN: VersionString("0.13.0"),
    "manylinux2014" + "aarch64" + WASMEDGE_TENSORFLOW_PLUGIN: VersionString("0.13.0"),
    "ubuntu20.04" + "x86_64" + WASMEDGE_TENSORFLOW_PLUGIN: VersionString("0.13.0"),
    "darwin" + "x86_64" + WASMEDGE_TENSORFLOW_LITE_PLUGIN: VersionString("0.13.0"),
    "darwin" + "arm64" + WASMEDGE_TENSORFLOW_LITE_PLUGIN: VersionString("0.13.0"),
    "manylinux2014"
    + "x86_64"
    + WASMEDGE_TENSORFLOW_LITE_PLUGIN: VersionString("0.13.0"),
    "manylinux2014"
    + "aarch64"
    + WASMEDGE_TENSORFLOW_LITE_PLUGIN: VersionString("0.13.0"),
    "ubuntu20.04" + "x86_64" + WASMEDGE_TENSORFLOW_LITE_PLUGIN: VersionString("0.13.0"),
    "darwin" + "x86_64" + WASMEDGE_IMAGE_PLUGIN: VersionString("0.13.0"),
    "darwin" + "arm64" + WASMEDGE_IMAGE_PLUGIN: VersionString("0.13.0"),
    "manylinux2014" + "x86_64" + WASMEDGE_IMAGE_PLUGIN: VersionString("0.13.0"),
    "manylinux2014" + "aarch64" + WASMEDGE_IMAGE_PLUGIN: VersionString("0.13.0"),
    "ubuntu20.04" + "x86_64" + WASMEDGE_IMAGE_PLUGIN: VersionString("0.13.0"),
    "darwin" + "x86_64" + WASI_LOGGING: VersionString("0.14.0"),
    "darwin" + "arm64" + WASI_LOGGING: VersionString("0.13.5"),
    "manylinux2014" + "aarch64" + WASI_LOGGING: VersionString("0.13.5"),
    "manylinux2014" + "x86_64" + WASI_LOGGING: VersionString("0.13.5"),
    "ubuntu20.04" + "x86_64" + WASI_LOGGING: VersionString("0.13.5"),
    "ubuntu20.04" + "aarch64" + WASI_LOGGING: VersionString("0.14.0"),
    "darwin" + "x86_64" + WASMEDGE_RUSTLS: VersionString("0.13.4"),
    "darwin" + "arm64" + WASMEDGE_RUSTLS: VersionString("0.13.4"),
    "manylinux2014" + "aarch64" + WASMEDGE_RUSTLS: VersionString("0.13.5"),
    "manylinux2014" + "x86_64" + WASMEDGE_RUSTLS: VersionString("0.13.4"),
    "ubuntu20.04" + "x86_64" + WASMEDGE_RUSTLS: VersionString("0.13.4"),
    "ubuntu20.04" + "aarch64" + WASMEDGE_RUSTLS: VersionString("0.13.5"),
    "ubuntu20.04" + "x86_64" + WASM_BPF: VersionString("0.13.2"),
    "manylinux2014" + "x86_64" + WASM_BPF: VersionString("0.13.2"),
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
CONST_PATH_NOT_EXIST_STR = "/DOES NOT EXIST;"

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
            export PATH="{0}/bin:$PATH"
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
            export {2}="{0}/{6}:${2}"
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
            export LIBRARY_PATH="{0}/{6}:$LIBRARY_PATH"
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
            export C_INCLUDE_PATH="{0}/include:$C_INCLUDE_PATH"
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
            export CPLUS_INCLUDE_PATH="{0}/include:$CPLUS_INCLUDE_PATH"
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
            CONST_shell_profile = join(HOME, "." + "zshenv")
        else:
            CONST_shell_profile = join(HOME, "." + SHELL + "_profile")

        if not exists(CONST_shell_config) and compat.platform != "Darwin":
            open(CONST_shell_config, "a").close()

        write_shell = False
        if compat.platform != "Darwin":
            with opened_w_error(CONST_shell_config, "r") as shell_config:
                if shell_config is not None:
                    if source_string not in shell_config.read():
                        write_shell = True
                else:
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
        elif compat.platform == "Darwin" and "zsh" in SHELL:
            open(CONST_shell_profile, "a").close()
            with opened_w_error(CONST_shell_profile, "r") as shell_config:
                if shell_config is not None:
                    if source_string not in shell_config.read():
                        write_shell = True
                else:
                    write_shell = True
            if write_shell:
                with opened_w_error(CONST_shell_profile, "a") as shell_profile:
                    if shell_profile is not None:
                        shell_profile.write(source_string)
                write_shell = False

    else:
        logging.error("Unknown shell found")
        return -1

    logging.info("shell configuration updated")
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


def check_nvcc(platform):
    if platform == "Linux":
        cmd = "/usr/local/cuda/bin/nvcc --version 2>/dev/null"
        output = run_shell_command(cmd)
        logging.debug("%s: %s", cmd, output)
        if "nvcc: NVIDIA (R) Cuda compiler driver" in output:
            return True
        else:
            logging.info("CUDA cannot be detected via nvcc")
            return False
    else:
        logging.info("CUDA is only supported on Linux")
    return False


def check_nvidia_smi(platform):
    if platform == "Linux":
        cmd = "nvidia-smi -q 2>/dev/null | grep CUDA | cut -f2 -d ':'"
        output = run_shell_command(cmd)
        logging.debug("%s: %s", cmd, output)
        if "12" in output:  # Check if CUDA 12.x is installed
            return True
        else:
            logging.info("CUDA 12.x cannot be detected via nvidia-smi")
            return False
    else:
        logging.info("CUDA is only supported on Linux")
    return False


def check_libcudart(platform):
    if platform == "Linux":
        cmd = "ldconfig -p | grep libcudart.so"
        output = run_shell_command(cmd)
        logging.debug("%s: %s", cmd, output)
        if "libcudart.so" in output:
            return True
        else:
            logging.info("Cannot find libcudart.so")
            return False
    return False


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


def install_tensorflow_extension(
    args,
    compat,
    download_tf_deps_=False,
    download_tf_lite_deps_=False,
):
    global CONST_release_pkg, CONST_lib_ext, CONST_lib_dir, CONST_env_path

    download_tf_deps = download_tf_deps_
    download_tf_lite_deps = download_tf_lite_deps_

    logging.debug(
        "install_tensorflow_extension: %s %s",
        download_tf_deps,
        download_tf_lite_deps,
    )

    if (
        not get_remote_version_availability(
            "second-state/WasmEdge-tensorflow-deps", args.tf_deps_version
        )
        and download_tf_deps
    ):
        logging.debug(
            "Tensorflow Deps extension version not found: {0}".format(
                args.tf_deps_version
            )
        )
        download_tf_deps = False

    if compat.machine == "aarch64":
        download_tf_deps = False
        logging.warning(
            "Cannot download WasmEdge Tensorflow, Tools & Deps because it is aarch64"
        )

    local_release_package = CONST_release_pkg

    # From WasmEdge 0.11.1, we have the Ubuntu release.
    # Installation of ubuntu version extensions when the ubuntu version of WasmEdge selected.
    if VersionString(args.version).compare("0.11.1") >= 0:
        local_release_package = compat.release_package_wasmedge
        logging.debug("Downloading dist package: {0}".format(local_release_package))

    if download_tf_deps:
        tf_deps_pkg = (
            "WasmEdge-tensorflow-deps-TF-"
            + args.tf_deps_version
            + "-"
            + CONST_release_pkg
        )

        logging.info("Downloading tensorflow-deps")
        download_url(
            CONST_urls[TENSORFLOW_DEPS], join(TEMP_PATH, tf_deps_pkg), show_progress
        )

        # Extract archive
        extract_archive(
            join(TEMP_PATH, tf_deps_pkg),
            join(args.path, CONST_lib_dir),
            join(TEMP_PATH, "WasmEdge-tensorflow-deps", CONST_lib_dir),
            env_file_path=CONST_env_path,
            remove_finished=True,
        )
        copytree(join(TEMP_PATH, "WasmEdge-tensorflow-deps"), args.path)

    if download_tf_lite_deps:
        tf_deps_lite_pkg = (
            "WasmEdge-tensorflow-deps-TFLite-"
            + args.tf_deps_version
            + "-"
            + CONST_release_pkg
        )

        logging.info("Downloading tensorflow-lite-deps")
        download_url(
            CONST_urls[TENSORFLOW_LITE_DEPS],
            join(TEMP_PATH, tf_deps_lite_pkg),
            show_progress,
        )

        # Extract archive
        extract_archive(
            join(TEMP_PATH, tf_deps_lite_pkg),
            join(args.path, CONST_lib_dir),
            join(TEMP_PATH, "WasmEdge-tensorflow-lite-deps", CONST_lib_dir),
            env_file_path=CONST_env_path,
            remove_finished=True,
        )

        copytree(join(TEMP_PATH, "WasmEdge-tensorflow-lite-deps"), args.path)

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

    return 0


def install_plugins(args, compat):
    global CONST_lib_dir
    url_root = "https://github.com/WasmEdge/WasmEdge/releases/download/"
    url_root += "$VERSION$/WasmEdge-plugin-$PLUGIN_NAME$-$VERSION$-$DIST$_$ARCH$.tar.gz"

    if len(args.plugins) >= 1:
        for plugin_name in args.plugins:
            # Reset the url_root, due to the wasi-nn-ggml plugin with the build number will change the url
            url_root = "https://github.com/WasmEdge/WasmEdge/releases/download/"
            url_root += (
                "$VERSION$/WasmEdge-plugin-$PLUGIN_NAME$-$VERSION$-$DIST$_$ARCH$.tar.gz"
            )
            plugin_version_supplied = None
            plugin_wasi_nn_ggml_bypass_check = False
            if plugin_name.find(":") != -1:
                plugin_name, plugin_version_supplied = plugin_name.split(":")

            # Deprecated rustls after 0.14.0
            # Only allow users to install rustls with 0.13.5
            if (
                plugin_name.startswith(WASMEDGE_RUSTLS)
                and compat.version.compare("0.13.5") != 0
            ):
                logging.warning("WasmEdge Rustls plugin is only available in 0.13.5")
                logging.warning("Please use -v 0.13.5 as a workaround")
                logging.warning("Skip installing WasmEdge Rustls plugin")
                continue

            # Deprecated wasi-logging after 0.14.1-rc.1
            if (
                plugin_name.startswith(WASI_LOGGING)
                and compat.version.compare("0.14.1-rc.1") != -1
            ):
                logging.warning(
                    "WASI-Logging plugin is bundled into libwasmedge in 0.14.1-rc.1"
                )
                logging.warning("No need to install the WASI-Logging plugin")
                continue

            # Split the WASI-NN-GGML plugin and the others
            if plugin_name.startswith(WASI_NN_GGML):
                # Re-write the plugin name if the build number is supplied
                # E.g. wasi_nn-ggml-b2330, wasi_nn-ggml-cuda-b2330, wasi_nn-ggml-cuda-11-b2330
                # "https://github.com/second-state/WASI-NN-GGML-PLUGIN-REGISTRY/raw/main/"
                # "$VERSION$/"
                # "$BUILD_NUMBER$/"
                # "WasmEdge-plugin"
                # "-$PLUGIN_NAME$"
                # "-$VERSION$"
                # "-$DIST$"
                # "_$ARCH$" # ".tar.gz"
                # If the build number is supplied, bypass the checks
                plugin_wasi_nn_ggml_bypass_check = True
                if plugin_name.startswith(WASI_NN_GGML) and "-b" in plugin_name:
                    [plugin_name, plugin_build_number] = plugin_name.split("-b")
                    url_root = "https://github.com/second-state/WASI-NN-GGML-PLUGIN-REGISTRY/raw/main/"
                    url_root += "$VERSION$/b$BUILD_NUMBER$/WasmEdge-plugin-$PLUGIN_NAME$-$VERSION$-$DIST$_$ARCH$.tar.gz"
                    url_root = url_root.replace("$BUILD_NUMBER$", plugin_build_number)

                # Re-write the plugin name if CUDA is available
                if plugin_name == WASI_NN_GGML and compat.cuda:
                    plugin_name = WASI_NN_GGML_CUDA

            # Normal plugin
            if (
                plugin_name not in PLUGINS_AVAILABLE
                and not plugin_wasi_nn_ggml_bypass_check
            ):
                logging.error(
                    "%s plugin not found, available names - %s",
                    plugin_name,
                    PLUGINS_AVAILABLE,
                )
                continue

            if (
                compat.dist + compat.machine + plugin_name not in SUPPORTTED_PLUGINS
                and not plugin_wasi_nn_ggml_bypass_check
            ):
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

                if (
                    WASMEDGE_TENSORFLOW_PLUGIN == plugin_name
                    and VersionString(args.version).compare("0.13.0") >= 0
                ):
                    if (
                        install_tensorflow_extension(
                            args, compat, download_tf_deps_=True
                        )
                        != 0
                    ):
                        logging.error("Error in installing tensorflow deps")
                    else:
                        logging.info("Tensorflow deps installed")

                if WASMEDGE_TENSORFLOW_LITE_PLUGIN == plugin_name:
                    if (
                        install_tensorflow_extension(
                            args, compat, download_tf_lite_deps_=True
                        )
                        != 0
                    ):
                        logging.error("Error in installing tensorflow deps")
                    else:
                        logging.info("Tensorflow deps installed")

                if WASI_NN_TENSORFLOW_LITE == plugin_name:
                    if (
                        install_tensorflow_extension(
                            args, compat, download_tf_lite_deps_=True
                        )
                        != 0
                    ):
                        logging.error("Error in installing tensorflow deps")
                    else:
                        logging.info("Tensorflow deps installed")

                plugin_url = (
                    url_root.replace("$PLUGIN_NAME$", plugin_name)
                    .replace("$VERSION$", plugin_version_supplied)
                    .replace("$DIST$", compat.dist)
                    .replace("$ARCH$", compat.machine)
                )
                logging.debug("Plugin URL: %s", plugin_url)

                logging.info("Downloading Plugin: " + plugin_name)
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

    local_release_package_tf = CONST_release_pkg

    # From WasmEdge 0.11.1, we have the Ubuntu release.
    # Installation of ubuntu version extensions when the ubuntu version of WasmEdge selected.
    if VersionString(args.tf_version).compare("0.11.1") >= 0:
        local_release_package_tf = compat.release_package_wasmedge
        logging.debug("Tensorflow release pkg: {0}".format(local_release_package_tf))

    CONST_urls = {
        WASMEDGE: "https://github.com/WasmEdge/WasmEdge/releases/download/{0}/WasmEdge-{0}-{1}".format(
            args.version, compat.release_package_wasmedge
        ),
        WASMEDGE_UNINSTALLER: "https://raw.githubusercontent.com/WasmEdge/WasmEdge/{0}/utils/uninstall.sh".format(
            args.uninstall_script_tag
        ),
        TENSORFLOW_DEPS: "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/{0}/WasmEdge-tensorflow-deps-TF-{0}-{1}".format(
            args.tf_deps_version, CONST_release_pkg
        ),
        TENSORFLOW_LITE_DEPS: "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/{0}/WasmEdge-tensorflow-deps-TFLite-{0}-{1}".format(
            args.tf_deps_version, CONST_release_pkg
        ),
    }


def run_shell_command(cmd):
    try:
        output = subprocess.check_output([cmd], shell=True)
        return output.decode("utf8").strip()
    except subprocess.CalledProcessError as e:
        if "Cannot detect installation path" in str(e.output):
            logging.warning("Uninstaller did not find previous installation")
        elif "ldconfig" in str(e.cmd):
            logging.debug("Cannot detect libcudart via ldconfig")
        elif "nvcc" in str(e.cmd):
            logging.debug("Cannot detect CUDA via nvcc")
        else:
            logging.error(
                "Exception on process - rc= %s output= %s command= %s",
                e.returncode,
                e.output,
                e.cmd,
            )

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
    ):
        self.platform = platform_  # Linux, Darwin
        self.machine = machine  # x86_64, arm
        self.version = VersionString(version)
        self.release_package = None
        self.install_package_name = None
        self.lib_extension = None
        self.ld_library_path = None
        self.dist = dist_
        self.release_package_wasmedge = None
        self.cuda = check_libcudart(self.platform) and (
            check_nvcc(self.platform) or check_nvidia_smi(self.platform)
        )

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
                # Only use Ubuntu when the arch is x86_64
                # See https://github.com/WasmEdge/WasmEdge/issues/2595#issuecomment-1592460709
                if sys.version_info[0] == 2:
                    __lsb_rel = run_shell_command(
                        "cat /etc/lsb-release 2>/dev/null | grep RELEASE"
                    )[-5:]
                    __platform_dist = platform.dist()
                    if (
                        VersionString(__platform_dist[1]).compare("20.04") >= 0
                        or VersionString(__lsb_rel).compare("20.04") >= 0
                    ):
                        # ARM-based Ubuntu 20.04 is supported after 0.13.5
                        if self.machine in ["x86_64", "amd64"] or (
                            self.machine in ["aarch64", "arm64"]
                            and self.version.compare("0.13.5") >= 0
                        ):
                            self.dist = "ubuntu20.04"
                        else:
                            self.dist = "manylinux2014"
                    else:
                        self.dist = "manylinux2014"
                elif sys.version_info[0] == 3:
                    __lsb_rel = run_shell_command(
                        "cat /etc/lsb-release 2>/dev/null | grep RELEASE"
                    )[-5:]
                    if VersionString(__lsb_rel).compare(
                        "20.04"
                    ) >= 0 or "Ubuntu 20.04" in run_shell_command(
                        "cat /etc/lsb_release 2>/dev/null | grep DESCRIPTION"
                    ):
                        # ARM-based Ubuntu 20.04 is supported after 0.13.5
                        if self.machine in ["x86_64", "amd64"] or (
                            self.machine in ["aarch64", "arm64"]
                            and self.version.compare("0.13.5") >= 0
                        ):
                            self.dist = "ubuntu20.04"
                        else:
                            self.dist = "manylinux2014"
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
        return "Platform:{0}\nMachine:{1}\nVersion:{2}\nDist:{3}\n".format(
            self.platform, self.machine, self.version, self.dist
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
    global CONST_env_path, CONST_release_pkg, CONST_ipkg, CONST_shell_config
    global CONST_shell_profile, CONST_lib_dir

    compat = Compat(
        version=args.version,
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

        if "all" in args.plugins:
            args.plugins = PLUGINS_AVAILABLE[:]
            logging.debug("Selected all of the available plugins: %s", args.plugins)

    if compat:
        logging.info("Compatible with current configuration")

        set_consts(args, compat)

        if compat.version.compare("0.13.0") == -1:
            logging.error("Please install the 0.13.0 or above versions.")
            exit(1)

        # Run uninstaller
        uninstaller_path = join(TEMP_PATH, "uninstall.sh")
        download_url(CONST_urls[WASMEDGE_UNINSTALLER], uninstaller_path)

        logging.info("Running Uninstaller")

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

        logging.info("Downloading WasmEdge")

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

        logging.info("Installing WasmEdge")
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
            logging.info("WasmEdge Successfully installed")
        else:
            logging.critical(
                "WasmEdge installation incorrect: {0}".format(wasmedge_output)
            )

        install_plugins(args, compat)

        ldconfig(args, compat)

        # Cleanup
        shutil.rmtree(TEMP_PATH)

        if CONST_shell_config is None:
            CONST_shell_config = CONST_PATH_NOT_EXIST_STR

        if CONST_shell_profile is None:
            CONST_shell_profile = CONST_PATH_NOT_EXIST_STR

        if exists(CONST_shell_config) and compat.platform != "Darwin":
            logging.info("Run:\nsource {0}".format(CONST_shell_config))
        elif exists(CONST_shell_profile):
            logging.info("Run:\nsource {0}".format(CONST_shell_profile))
        else:
            logging.info("Please source the env file: %s", join(args.path, "env"))
    else:
        reraise(Exception("Incompatible with your machine\n{0}".format(compat)))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="WasmEdge installation, uninstallation and plugins install"
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
        default=logging.INFO,
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
        help="(experimental option)Install Supported Plugins - ["
        + ",".join(PLUGINS_AVAILABLE)
        + "]. Example"
        " '--plugins wasi_crypto:0.11.0'"
        " '--plugins wasi_crypto'"
        " '--plugins all' [Downloads all the supported plugins]",
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
        if VersionString(args.version).compare("0.12.0") == -1:
            args.tf_deps_version = "TF-2.6.0"
        elif VersionString(args.version).compare("0.13.0") == -1:
            args.tf_deps_version = "TF-2.6.0-CC"
        elif VersionString(args.version).compare("0.13.0") >= 0:
            args.tf_deps_version = "TF-2.12.0-CC"
        else:
            reraise("Should not reach here")

    logging.debug("Python Version: %s", sys.version_info)
    main(args)
