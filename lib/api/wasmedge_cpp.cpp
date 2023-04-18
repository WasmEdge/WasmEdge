#include "wasmedge/wasmedge.hh"
#include "ast/type.h"

namespace WasmEdge {

// >>>>>>>> WasmEdge Data Structures >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

bool Limit::operator==(const Limit &Lim) {
  return this->HasMax == Lim.HasMax && this->Shared == Lim.Shared &&
         this->Min == Lim.Min && this->Max == Lim.Max;
}

// >>>>>>>> WasmEdge FunctionType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class FunctionType::FunctionTypeContext {
  std::unique_ptr<AST::FunctionType> FuncTypeCxt;
public:
  FunctionTypeContext(std::unique_ptr<AST::FunctionType> FuncType)
    : FuncTypeCxt(std::move(FuncType)) {}
  ~FunctionTypeContext() = default;

  std::vector<ValType> &GetParameters() const {
    if (FuncTypeCxt) {
      return FuncTypeCxt->getParamTypes();
    }
    auto List = std::make_unique<std::vector<ValType>>();
    return *List;
  }

  std::vector<ValType> &GetReturns() const {
    if (FuncTypeCxt) {
      return FuncTypeCxt->getReturnTypes();
    }
    auto List = std::make_unique<std::vector<ValType>>();
    return *List;
  }
};

FunctionType::FunctionType(const std::vector<ValType> &ParamList,
                           const std::vector<ValType> &ReturnList) {
  auto FuncTypeCxt = std::make_unique<AST::FunctionType>();
  if (!ParamList.empty()) {
    FuncTypeCxt->getParamTypes().reserve(ParamList.size());
    std::copy(ParamList.begin(), ParamList.end(),
              std::back_inserter(ParamList));
  }
  if (!ReturnList.empty()) {
    FuncTypeCxt->getReturnTypes().reserve(ReturnList.size());
    std::copy(ReturnList.begin(), ReturnList.end(),
              std::back_inserter(ReturnList));
  }

  this->Cxt = std::make_unique<FunctionType::FunctionTypeContext>(
                              std::move(FuncTypeCxt));
}

const std::vector<ValType> &FunctionType::GetParameters() {
  return Cxt->GetParameters();
}

const std::vector<ValType> &FunctionType::GetReturns() {
  return Cxt->GetReturns();
}

// <<<<<<<< WasmEdge FunctionType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// <<<<<<<< WasmEdge Data Structures <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
