#include "WasmEdge.hpp"

/* --------------- Result ----------------------------------------*/

pysdk::result::result() {}

pysdk::result::result(int &code) { Res.Code = code; }

pysdk::result::result(WasmEdge_Result res) { Res = res; }

pysdk::result::operator bool() { return WasmEdge_ResultOK(Res); }

const char *pysdk::result::message() { return WasmEdge_ResultGetMessage(Res); }

int pysdk::result::get_code() { return WasmEdge_ResultGetCode(Res); }

WasmEdge_Result pysdk::result::get() { return Res; }
/* --------------- Result End ----------------------------------------*/
