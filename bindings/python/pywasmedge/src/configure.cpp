#include "WasmEdge.hpp"

/* --------------- Configure -------------------------------- */

pysdk::Configure::Configure() { ConfCxt = WasmEdge_ConfigureCreate(); }

pysdk::Configure::~Configure() { WasmEdge_ConfigureDelete(ConfCxt); }

WasmEdge_ConfigureContext *pysdk::Configure::get() { return this->ConfCxt; }

void pysdk::Configure::AddProposal(WasmEdge_Proposal &prop) {
  WasmEdge_ConfigureAddProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::RemoveProposal(WasmEdge_Proposal &prop) {
  WasmEdge_ConfigureRemoveProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::AddHostRegistration(WasmEdge_HostRegistration &hr) {
  WasmEdge_ConfigureAddHostRegistration(ConfCxt,
                                        (::WasmEdge_HostRegistration)hr);
}

void pysdk::Configure::RemoveHostRegistration(WasmEdge_HostRegistration &hr) {
  WasmEdge_ConfigureRemoveHostRegistration(ConfCxt,
                                           (::WasmEdge_HostRegistration)hr);
}

WasmEdge_CompilerOptimizationLevel
pysdk::Configure::CompilerGetOptimizationLevel() {
  return WasmEdge_ConfigureCompilerGetOptimizationLevel(ConfCxt);
}

WasmEdge_CompilerOutputFormat pysdk::Configure::CompilerGetOutputFormat() {
  return WasmEdge_ConfigureCompilerGetOutputFormat(ConfCxt);
}

bool pysdk::Configure::CompilerIsDumpIR() {
  return WasmEdge_ConfigureCompilerIsDumpIR(ConfCxt);
}

bool pysdk::Configure::CompilerIsGenericBinary() {
  return WasmEdge_ConfigureCompilerIsGenericBinary(ConfCxt);
}

bool pysdk::Configure::CompilerIsInterruptible() {
  return WasmEdge_ConfigureCompilerIsInterruptible(ConfCxt);
}

void pysdk::Configure::CompilerSetDumpIR(bool &IsDump) {
  WasmEdge_ConfigureCompilerSetDumpIR(ConfCxt, IsDump);
}

void pysdk::Configure::CompilerSetGenericBinary(bool &IsGeneric) {
  WasmEdge_ConfigureCompilerSetGenericBinary(ConfCxt, IsGeneric);
}

void pysdk::Configure::CompilerSetInterruptible(bool &IsInterruptible) {
  WasmEdge_ConfigureCompilerSetInterruptible(ConfCxt, IsInterruptible);
}

void pysdk::Configure::CompilerSetOptimizationLevel(
    WasmEdge_CompilerOptimizationLevel &level) {
  WasmEdge_ConfigureCompilerSetOptimizationLevel(ConfCxt, level);
}

void pysdk::Configure::CompilerSetOutputFormat(
    WasmEdge_CompilerOutputFormat &fmt) {
  WasmEdge_ConfigureCompilerSetOutputFormat(ConfCxt, fmt);
}

uint32_t pysdk::Configure::GetMaxMemoryPage() {
  return WasmEdge_ConfigureGetMaxMemoryPage(ConfCxt);
}

void pysdk::Configure::SetMaxMemoryPage(uint32_t &max_memory) {
  WasmEdge_ConfigureSetMaxMemoryPage(ConfCxt, max_memory);
}

bool pysdk::Configure::HasHostRegistration(WasmEdge_HostRegistration &reg) {
  return WasmEdge_ConfigureHasHostRegistration(ConfCxt, reg);
}

bool pysdk::Configure::HasProposal(WasmEdge_Proposal &prop) {
  return WasmEdge_ConfigureHasProposal(ConfCxt, prop);
}

bool pysdk::Configure::StatisticsIsCostMeasuring() {
  return WasmEdge_ConfigureStatisticsIsCostMeasuring(ConfCxt);
}

bool pysdk::Configure::StatisticsIsInstructionCounting() {
  return WasmEdge_ConfigureStatisticsIsInstructionCounting(ConfCxt);
}

bool pysdk::Configure::StatisticsIsTimeMeasuring() {
  return WasmEdge_ConfigureStatisticsIsTimeMeasuring(ConfCxt);
}

void pysdk::Configure::StatisticsSetCostMeasuring(bool &IsMeasure) {
  WasmEdge_ConfigureStatisticsSetCostMeasuring(ConfCxt, IsMeasure);
}

void pysdk::Configure::StatisticsSetInstructionCounting(bool &val) {
  WasmEdge_ConfigureStatisticsSetInstructionCounting(ConfCxt, val);
}

void pysdk::Configure::StatisticsSetTimeMeasuring(bool &val) {
  WasmEdge_ConfigureStatisticsSetTimeMeasuring(ConfCxt, val);
}
/* --------------- Configure End -------------------------------- */
