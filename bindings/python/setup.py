"""Python setup.py for WasmEdge package"""
import io
import os
from setuptools import find_packages, setup, Extension

def find(name, path=os.path.join(os.environ['HOME'], ".wasmedge")):
    ret = None
    for root, dirs, files in os.walk(path):
        if name in files:
            ret = os.path.join(root)

    if ret == None:
        raise Exception(name+" is needed, please provide path")
    
    return ret

def read(*paths, **kwargs):
    """Read the contents of a text file safely.
    >>> read("WasmEdge", "VERSION")
    '0.1.0'
    >>> read("README.md")
    ...
    """

    content = ""
    with io.open(
        os.path.join(os.path.dirname(__file__), *paths),
        encoding=kwargs.get("encoding", "utf8"),
    ) as open_file:
        content = open_file.read().strip()
    return content


def read_requirements(path):
    return [
        line.strip()
        for line in read(path).split("\n")
        if not line.startswith(('"', "#", "-", "git+"))
    ]

wasmedge_include = find("wasmedge.h")

wasmedge_libs = find("libwasmedge_c.so")

WasmEdge_Module = Extension('WasmEdge',
                            sources=['pywasmedge/src/WasmEdge.c'],
                            include_dirs=[
                                'pywasmedge/include/',wasmedge_include ],
                            libraries=['wasmedge_c'],
                            library_dirs=[wasmedge_libs],
                            language='c'
                            )

setup(
    name="WasmEdge",
    version=read("VERSION"),
    description="Python interface for WasmEdge",
    url="https://github.com/WasmEdge/WasmEdge/",
    long_description=read("README.md"),
    long_description_content_type="text/markdown",
    author="Shreyas Atre",
    packages=find_packages(exclude=["tests", ".github"]),
    install_requires=read_requirements("requirements.txt"),
    # entry_points={
    #     "console_scripts": ["WasmEdge = WasmEdge.__main__:main"]
    # },
    extras_require={"test": read_requirements("requirements-test.txt")},
    ext_modules=[WasmEdge_Module]
)
