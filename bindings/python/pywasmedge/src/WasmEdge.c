#include "WasmEdge.h"

static PyObject *WasmEdge_er;

/* Example from
 * https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md#wasmedge-vm
 */
static PyObject *run(const char *path, const char *function_name) {
  /*Create the configure context and add the WASI support.* /
  /* This step is not necessary unless you need WASI support. */
  PyObject *ret = NULL;
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(ConfCxt,
                                        WasmEdge_HostRegistration_Wasi);
  /* The configure and store context to the VM creation can be NULL. */
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);

  /* The parameters and returns arrays. */
  WasmEdge_Value Params[1] = {WasmEdge_ValueGenI32(32)};
  WasmEdge_Value Returns[1];
  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString(function_name);
  /* Run the WASM function from file. */
  WasmEdge_Result Res =
      WasmEdge_VMRunWasmFromFile(VMCxt, path, FuncName, Params, 1, Returns, 1);

  if (WasmEdge_ResultOK(Res)) {
    ret = Py_BuildValue("k", WasmEdge_ValueGetI32(Returns[0]));
  } else {
    char buf[64];
    sprintf(buf, "Error message: %s\n", WasmEdge_ResultGetMessage(Res));
    PyErr_SetString(WasmEdge_er, buf);
    return NULL;
  }

  /* Resources deallocations. */
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  WasmEdge_StringDelete(FuncName);
  return ret;
}

// A static function which takes PyObjects arguments and returns a PyObject
// result
static PyObject *py_run(PyObject *self, PyObject *args) {
  const char *path, *function_name;
  if (!PyArg_ParseTuple(
          args, "ss", &path,
          &function_name)) // Validate and parse the arguments received to
                           // function so that its usable by C
    return NULL;
  return run(path, function_name); // Get result from C, wrap it up with a
                                   // PyObject and return it
}

// Define a collection of methods callable from our module
static PyMethodDef WasmEdge_Methods[] = {
    {"py_run", py_run, METH_VARARGS, "Runs specified .wasm file"}};

// Module definition
static struct PyModuleDef wasmedge_module = {PyModuleDef_HEAD_INIT, "WasmEdge",
                                             "WasmEdge SDK for Python", -1,
                                             WasmEdge_Methods};

// This method is called when you import your code in python. It instantiates
// the module and returns a reference to it
PyMODINIT_FUNC PyInit_WasmEdge(void) {
  return PyModule_Create(&wasmedge_module);
}
