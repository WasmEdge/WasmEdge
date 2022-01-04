#include "WasmEdge.hpp"

/* --------------- Configure -------------------------------- */

pysdk::Configure::Configure() { ConfCxt = WasmEdge_ConfigureCreate(); }

pysdk::Configure::~Configure() { WasmEdge_ConfigureDelete(ConfCxt); }

WasmEdge_ConfigureContext *pysdk::Configure::get() { return this->ConfCxt; }

void pysdk::Configure::add(WasmEdge_Proposal prop) {
  WasmEdge_ConfigureAddProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::remove(WasmEdge_Proposal prop) {
  WasmEdge_ConfigureRemoveProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::add(WasmEdge_HostRegistration hr) {
  WasmEdge_ConfigureAddHostRegistration(ConfCxt,
                                        (::WasmEdge_HostRegistration)hr);
}

void pysdk::Configure::remove(WasmEdge_HostRegistration hr) {
  WasmEdge_ConfigureRemoveHostRegistration(ConfCxt,
                                           (::WasmEdge_HostRegistration)hr);
}

void pysdk::Configure::set_max_paging(uint32_t max_memory) {
  WasmEdge_ConfigureSetMaxMemoryPage(ConfCxt, max_memory);
}

uint32_t pysdk::Configure::get_max_paging() {
  return WasmEdge_ConfigureGetMaxMemoryPage(ConfCxt);
}

void pysdk::Configure::set_opt_level(WasmEdge_CompilerOptimizationLevel level) {
  WasmEdge_ConfigureCompilerSetOptimizationLevel(ConfCxt, level);
}

WasmEdge_CompilerOptimizationLevel pysdk::Configure::get_opt_level() {
  return WasmEdge_ConfigureCompilerGetOptimizationLevel(ConfCxt);
}

/* --------------- Configure End -------------------------------- */
