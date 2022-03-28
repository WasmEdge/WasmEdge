#include "WasmEdge.hpp"

/* --------------- import_object End ----------------------------------------*/

pysdk::import_object::import_object(std::string &name) {
  ModCxt =
      WasmEdge_ImportObjectCreate(WasmEdge_StringCreateByCString(name.c_str()));
}

pysdk::import_object::import_object(pybind11::tuple args, pybind11::tuple envs,
                                    pybind11::tuple preopens) {
  auto const arg_len = pybind11::len(args);
  auto const env_len = pybind11::len(envs);
  auto const pre_len = pybind11::len(preopens);
  char *Args[arg_len], *Envs[env_len], *PreOpens[pre_len];
  for (size_t i = 0; i < arg_len; i++) {
    auto const str = args[i].cast<std::string>();
    Args[i] = new char[str.length() + 1];
    strcpy(Args[i], str.c_str());
  }
  for (size_t i = 0; i < env_len; i++) {
    auto const str = envs[i].cast<std::string>();
    Envs[i] = new char[str.length() + 1];
    strcpy(Envs[i], str.c_str());
  }
  for (size_t i = 0; i < pre_len; i++) {
    auto const str = preopens[i].cast<std::string>();
    PreOpens[i] = new char[str.length() + 1];
    strcpy(PreOpens[i], str.c_str());
  }
  ModCxt = WasmEdge_ImportObjectCreateWASI(Args, arg_len, Envs, env_len,
                                           PreOpens, pre_len);
}

pysdk::import_object::import_object(pybind11::tuple commands, bool &val) {
  auto const arg_len = pybind11::len(commands);
  char *Args[arg_len];
  for (size_t i = 0; i < arg_len; i++) {
    auto const str = commands[i].cast<std::string>();
    Args[i] = new char[str.length() + 1];
    strcpy(Args[i], str.c_str());
  }
  ModCxt = WasmEdge_ImportObjectCreateWasmEdgeProcess(Args, arg_len, val);
}

pysdk::import_object::import_object(WasmEdge_ImportObjectContext *cxt) {
  ModCxt = cxt;
}

void pysdk::import_object::AddFunction(std::string &name,
                                       pysdk::Function &func) {
  WasmEdge_String function_name = WasmEdge_StringCreateByCString(name.c_str());
  WasmEdge_ImportObjectAddFunction(ModCxt, function_name, func.get());
  WasmEdge_StringDelete(function_name);
}

pysdk::import_object::~import_object() {}

WasmEdge_ImportObjectContext *pysdk::import_object::get() { return ModCxt; }

void pysdk::import_object::AddGlobal(std::string &str, pysdk::Global &glob) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  WasmEdge_ImportObjectAddGlobal(ModCxt, name, glob.get());
  WasmEdge_StringDelete(name);
}

void pysdk::import_object::AddMemory(std::string &str, pysdk::Memory &mem) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  WasmEdge_ImportObjectAddMemory(ModCxt, name, mem.get());
  WasmEdge_StringDelete(name);
}

void pysdk::import_object::AddTable(std::string &str, pysdk::Table &tab) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  WasmEdge_ImportObjectAddTable(ModCxt, name, tab.get());
  WasmEdge_StringDelete(name);
}

void pysdk::import_object::InitWASI(pybind11::tuple args, pybind11::tuple envs,
                                    pybind11::tuple preopens) {
  auto const arg_len = pybind11::len(args);
  auto const env_len = pybind11::len(envs);
  auto const pre_len = pybind11::len(preopens);
  char *Args[arg_len], *Envs[env_len], *PreOpens[pre_len];
  for (size_t i = 0; i < arg_len; i++) {
    auto const str = args[i].cast<std::string>();
    Args[i] = new char[str.length() + 1];
    strcpy(Args[i], str.c_str());
  }
  for (size_t i = 0; i < env_len; i++) {
    auto const str = envs[i].cast<std::string>();
    Envs[i] = new char[str.length() + 1];
    strcpy(Envs[i], str.c_str());
  }
  for (size_t i = 0; i < pre_len; i++) {
    auto const str = preopens[i].cast<std::string>();
    PreOpens[i] = new char[str.length() + 1];
    strcpy(PreOpens[i], str.c_str());
  }
  WasmEdge_ImportObjectInitWASI(ModCxt, Args, arg_len, Envs, env_len, PreOpens,
                                pre_len);
}

void pysdk::import_object::InitWasmEdgeProcess(pybind11::tuple commands,
                                               bool &val) {
  auto const arg_len = pybind11::len(commands);
  char *Args[arg_len];
  for (size_t i = 0; i < arg_len; i++) {
    auto const str = commands[i].cast<std::string>();
    Args[i] = new char[str.length() + 1];
    strcpy(Args[i], str.c_str());
  }
  WasmEdge_ImportObjectInitWasmEdgeProcess(ModCxt, Args, arg_len, val);
}

uint32_t pysdk::import_object::WASIGetExitCode() {
  return WasmEdge_ImportObjectWASIGetExitCode(ModCxt);
}
/* --------------- import_object End ----------------------------------------*/

/* --------------- ImportTypeContext ----------------------------------------*/
pysdk::ImportTypeContext::ImportTypeContext() {}

pysdk::ImportTypeContext::ImportTypeContext(WasmEdge_ImportTypeContext *cxt) {
  Cxt = cxt;
}

pysdk::ImportTypeContext::~ImportTypeContext() {}

std::string pysdk::ImportTypeContext::get_external_name() {
  const WasmEdge_String name = WasmEdge_ImportTypeGetExternalName(
      const_cast<const WasmEdge_ImportTypeContext *>(Cxt));
  char buf[name.Length];
  WasmEdge_StringCopy(name, buf, name.Length);
  return std::string(buf);
}

WasmEdge_ExternalType pysdk::ImportTypeContext::get_external_type() {
  return WasmEdge_ImportTypeGetExternalType(
      const_cast<const WasmEdge_ImportTypeContext *>(Cxt));
}

pysdk::FunctionTypeContext
pysdk::ImportTypeContext::get_function_type_cxt(pysdk::ASTModuleCxt &ast_cxt) {
  return pysdk::FunctionTypeContext(const_cast<WasmEdge_FunctionTypeContext *>(
      WasmEdge_ImportTypeGetFunctionType(
          const_cast<const WasmEdge_ASTModuleContext *>(ast_cxt.get()), Cxt)));
}

pysdk::GlobalTypeCxt
pysdk::ImportTypeContext::get_global_type_cxt(pysdk::ASTModuleCxt &ast_cxt) {
  return pysdk::GlobalTypeCxt(WasmEdge_ImportTypeGetGlobalType(
      const_cast<const WasmEdge_ASTModuleContext *>(ast_cxt.get()),
      const_cast<const WasmEdge_ImportTypeContext *>(Cxt)));
}

WasmEdge_ImportTypeContext *pysdk::ImportTypeContext::get() { return Cxt; }

pysdk::MemoryTypeCxt
pysdk::ImportTypeContext::GetMemoryType(pysdk::ASTModuleCxt &ast_cxt) {
  return pysdk::MemoryTypeCxt(
      WasmEdge_ImportTypeGetMemoryType(ast_cxt.get(), Cxt));
}

std::string pysdk::ImportTypeContext::GetModuleName() {
  WasmEdge_String str = WasmEdge_ImportTypeGetModuleName(Cxt);
  char temp[str.Length];
  WasmEdge_StringCopy(str, temp, str.Length);
  return std::string(temp);
}

pysdk::TableTypeCxt
pysdk::ImportTypeContext::GetTableType(pysdk::ASTModuleCxt &ast) {
  return pysdk::TableTypeCxt(WasmEdge_ImportTypeGetTableType(ast.get(), Cxt));
}
/* --------------- ImportTypeContext End -----------------------------------*/
