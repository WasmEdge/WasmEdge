#include "WasmEdge.hpp"

/* --------------- Configure -------------------------------- */

pysdk::Configure::Configure() { context = WasmEdge_ConfigureCreate(); }

pysdk::Configure::~Configure() {
  if (_del)
    WasmEdge_ConfigureDelete(context);
}

void pysdk::Configure::AddProposal(WasmEdge_Proposal &prop) {
  WasmEdge_ConfigureAddProposal(context, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::RemoveProposal(WasmEdge_Proposal &prop) {
  WasmEdge_ConfigureRemoveProposal(context, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::AddHostRegistration(WasmEdge_HostRegistration &hr) {
  WasmEdge_ConfigureAddHostRegistration(context,
                                        (::WasmEdge_HostRegistration)hr);
}

void pysdk::Configure::RemoveHostRegistration(WasmEdge_HostRegistration &hr) {
  WasmEdge_ConfigureRemoveHostRegistration(context,
                                           (::WasmEdge_HostRegistration)hr);
}

WasmEdge_CompilerOptimizationLevel
pysdk::Configure::CompilerGetOptimizationLevel() {
  return WasmEdge_ConfigureCompilerGetOptimizationLevel(context);
}

WasmEdge_CompilerOutputFormat pysdk::Configure::CompilerGetOutputFormat() {
  return WasmEdge_ConfigureCompilerGetOutputFormat(context);
}

bool pysdk::Configure::CompilerIsDumpIR() {
  return WasmEdge_ConfigureCompilerIsDumpIR(context);
}

bool pysdk::Configure::CompilerIsGenericBinary() {
  return WasmEdge_ConfigureCompilerIsGenericBinary(context);
}

bool pysdk::Configure::CompilerIsInterruptible() {
  return WasmEdge_ConfigureCompilerIsInterruptible(context);
}

void pysdk::Configure::CompilerSetDumpIR(bool &IsDump) {
  WasmEdge_ConfigureCompilerSetDumpIR(context, IsDump);
}

void pysdk::Configure::CompilerSetGenericBinary(bool &IsGeneric) {
  WasmEdge_ConfigureCompilerSetGenericBinary(context, IsGeneric);
}

void pysdk::Configure::CompilerSetInterruptible(bool &IsInterruptible) {
  WasmEdge_ConfigureCompilerSetInterruptible(context, IsInterruptible);
}

void pysdk::Configure::CompilerSetOptimizationLevel(
    WasmEdge_CompilerOptimizationLevel &level) {
  WasmEdge_ConfigureCompilerSetOptimizationLevel(context, level);
}

void pysdk::Configure::CompilerSetOutputFormat(
    WasmEdge_CompilerOutputFormat &fmt) {
  WasmEdge_ConfigureCompilerSetOutputFormat(context, fmt);
}

uint32_t pysdk::Configure::GetMaxMemoryPage() {
  return WasmEdge_ConfigureGetMaxMemoryPage(context);
}

void pysdk::Configure::SetMaxMemoryPage(uint32_t &max_memory) {
  WasmEdge_ConfigureSetMaxMemoryPage(context, max_memory);
}

bool pysdk::Configure::HasHostRegistration(WasmEdge_HostRegistration &reg) {
  return WasmEdge_ConfigureHasHostRegistration(context, reg);
}

bool pysdk::Configure::HasProposal(WasmEdge_Proposal &prop) {
  return WasmEdge_ConfigureHasProposal(context, prop);
}

bool pysdk::Configure::StatisticsIsCostMeasuring() {
  return WasmEdge_ConfigureStatisticsIsCostMeasuring(context);
}

bool pysdk::Configure::StatisticsIsInstructionCounting() {
  return WasmEdge_ConfigureStatisticsIsInstructionCounting(context);
}

bool pysdk::Configure::StatisticsIsTimeMeasuring() {
  return WasmEdge_ConfigureStatisticsIsTimeMeasuring(context);
}

void pysdk::Configure::StatisticsSetCostMeasuring(bool &IsMeasure) {
  WasmEdge_ConfigureStatisticsSetCostMeasuring(context, IsMeasure);
}

void pysdk::Configure::StatisticsSetInstructionCounting(bool &val) {
  WasmEdge_ConfigureStatisticsSetInstructionCounting(context, val);
}

void pysdk::Configure::StatisticsSetTimeMeasuring(bool &val) {
  WasmEdge_ConfigureStatisticsSetTimeMeasuring(context, val);
}
/* --------------- Configure End -------------------------------- */
