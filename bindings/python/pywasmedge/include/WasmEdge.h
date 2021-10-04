#ifndef PY_WASMEDGE_H
#define PY_WASMEDGE_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>
#include <wasmedge.h>

static PyObject *run(const char *path, const char *function_name);

static PyObject *py_run(PyObject *self, PyObject *args);

static PyObject *version();

PyMODINIT_FUNC PyInit_WasmEdge(void);

#endif // PY_WASMEDGE_H