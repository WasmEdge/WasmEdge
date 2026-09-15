#pragma once

#include "ast/component/type.h"
#include "common/types.h"
#include "runtime/instance/component/component.h"
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace Driver {

inline void consumeWhitespace(std::string_view &Input) {
  while (!Input.empty() &&
         std::isspace(static_cast<unsigned char>(Input.front()))) {
    Input.remove_prefix(1);
  }
}

inline bool consumeChar(std::string_view &Input, char C) {
  consumeWhitespace(Input);
  if (!Input.empty() && Input.front() == C) {
    Input.remove_prefix(1);
    return true;
  }
  return false;
}

inline std::optional<std::string> parseStringLiteral(std::string_view &Input) {
  consumeWhitespace(Input);
  if (Input.empty() || Input.front() != '"') {
    return std::nullopt;
  }
  Input.remove_prefix(1);
  std::string Result;
  bool Escaped = false;
  while (!Input.empty()) {
    char C = Input.front();
    Input.remove_prefix(1);
    if (Escaped) {
      Result.push_back(C);
      Escaped = false;
    } else if (C == '\\') {
      Escaped = true;
    } else if (C == '"') {
      return Result;
    } else {
      Result.push_back(C);
    }
  }
  return std::nullopt;
}

inline std::optional<std::string> parseIdentifier(std::string_view &Input) {
  consumeWhitespace(Input);
  if (Input.empty() ||
      !(std::isalpha(static_cast<unsigned char>(Input.front())) ||
        Input.front() == '_' || Input.front() == '-')) {
    return std::nullopt;
  }
  size_t Len = 0;
  while (Len < Input.size() &&
         (std::isalnum(static_cast<unsigned char>(Input[Len])) ||
          Input[Len] == '_' || Input[Len] == '-')) {
    Len++;
  }
  std::string Id(Input.substr(0, Len));
  Input.remove_prefix(Len);
  return Id;
}

inline std::optional<ComponentValVariant>
parseComponentValue(std::string_view &Input, const ComponentValType &ValTy,
                    const Runtime::Instance::ComponentInstance *CompInst) {
  consumeWhitespace(Input);

  if (ValTy.isPrimValType()) {
    auto PrimTy = ValTy.getCode();
    switch (PrimTy) {
    case ComponentTypeCode::Bool: {
      auto Id = parseIdentifier(Input);
      if (Id == "true")
        return true;
      if (Id == "false")
        return false;
      return std::nullopt;
    }
    case ComponentTypeCode::S8:
    case ComponentTypeCode::S16:
    case ComponentTypeCode::S32:
    case ComponentTypeCode::U8:
    case ComponentTypeCode::U16:
    case ComponentTypeCode::U32:
    case ComponentTypeCode::S64:
    case ComponentTypeCode::U64:
    case ComponentTypeCode::F32:
    case ComponentTypeCode::F64: {
      // Parse a numeric token
      size_t Len = 0;
      while (Len < Input.size() &&
             (std::isdigit(static_cast<unsigned char>(Input[Len])) ||
              Input[Len] == '-' || Input[Len] == '.' || Input[Len] == '+' ||
              Input[Len] == 'e' || Input[Len] == 'E' || Input[Len] == 'x' ||
              Input[Len] == 'X' || (Input[Len] >= 'a' && Input[Len] <= 'f') ||
              (Input[Len] >= 'A' && Input[Len] <= 'F'))) {
        Len++;
      }
      std::string NumStr(Input.substr(0, Len));
      Input.remove_prefix(Len);
      try {
        if (PrimTy == ComponentTypeCode::S8)
          return static_cast<int8_t>(std::stoi(NumStr, nullptr, 0));
        if (PrimTy == ComponentTypeCode::U8)
          return static_cast<uint8_t>(std::stoul(NumStr, nullptr, 0));
        if (PrimTy == ComponentTypeCode::S16)
          return static_cast<int16_t>(std::stoi(NumStr, nullptr, 0));
        if (PrimTy == ComponentTypeCode::U16)
          return static_cast<uint16_t>(std::stoul(NumStr, nullptr, 0));
        if (PrimTy == ComponentTypeCode::S32)
          return static_cast<int32_t>(std::stoi(NumStr, nullptr, 0));
        if (PrimTy == ComponentTypeCode::U32)
          return static_cast<uint32_t>(std::stoul(NumStr, nullptr, 0));
        if (PrimTy == ComponentTypeCode::S64)
          return static_cast<int64_t>(std::stoll(NumStr, nullptr, 0));
        if (PrimTy == ComponentTypeCode::U64)
          return static_cast<uint64_t>(std::stoull(NumStr, nullptr, 0));
        if (PrimTy == ComponentTypeCode::F32)
          return std::stof(NumStr);
        if (PrimTy == ComponentTypeCode::F64)
          return std::stod(NumStr);
      } catch (...) {
        return std::nullopt;
      }
      return std::nullopt;
    }
    case ComponentTypeCode::Char: {
      auto Str = parseStringLiteral(Input);
      if (Str && Str->size() > 0)
        return static_cast<uint32_t>(Str->front());
      return std::nullopt;
    }
    case ComponentTypeCode::String: {
      auto Str = parseStringLiteral(Input);
      if (Str)
        return *Str;
      return std::nullopt;
    }
    default:
      return std::nullopt;
    }
  } else {
    // TypeIndex
    if (!CompInst)
      return std::nullopt;
    uint32_t TypeIdx = ValTy.getTypeIndex();
    const auto *DefTy = CompInst->getType(TypeIdx);
    if (!DefTy || !DefTy->isDefValType())
      return std::nullopt;
    const auto &DVT = DefTy->getDefValType();

    if (DVT.isListTy()) {
      if (!consumeChar(Input, '['))
        return std::nullopt;
      ListVal L;
      const auto &ElemTy = DVT.getList().ValTy;
      while (!consumeChar(Input, ']')) {
        auto Elem = parseComponentValue(Input, ElemTy, CompInst);
        if (!Elem)
          return std::nullopt;
        L.Elements.push_back(std::move(*Elem));
        consumeChar(Input, ',');
      }
      return makeComponentVal(std::move(L));
    } else if (DVT.isRecordTy()) {
      if (!consumeChar(Input, '{'))
        return std::nullopt;
      RecordVal R;
      const auto &Fields = DVT.getRecord().LabelTypes;
      while (!consumeChar(Input, '}')) {
        auto FieldName = parseIdentifier(Input);
        if (!FieldName)
          return std::nullopt;
        if (!consumeChar(Input, ':'))
          return std::nullopt;
        // Find field type
        const ComponentValType *FieldTy = nullptr;
        for (const auto &F : Fields) {
          if (F.getLabel() == *FieldName) {
            FieldTy = &F.getValType();
            break;
          }
        }
        if (!FieldTy)
          return std::nullopt;
        auto Val = parseComponentValue(Input, *FieldTy, CompInst);
        if (!Val)
          return std::nullopt;
        R.Fields.push_back({*FieldName, std::move(*Val)});
        consumeChar(Input, ',');
      }
      return makeComponentVal(std::move(R));
    } else if (DVT.isTupleTy()) {
      if (!consumeChar(Input, '('))
        return std::nullopt;
      TupleVal T;
      const auto &Types = DVT.getTuple().Types;
      for (const auto &Ty : Types) {
        auto Val = parseComponentValue(Input, Ty, CompInst);
        if (!Val)
          return std::nullopt;
        T.Values.push_back(std::move(*Val));
        consumeChar(Input, ',');
      }
      if (!consumeChar(Input, ')'))
        return std::nullopt;
      return makeComponentVal(std::move(T));
    } else if (DVT.isVariantTy()) {
      auto CaseName = parseIdentifier(Input);
      if (!CaseName)
        return std::nullopt;
      const auto &Cases = DVT.getVariant().Cases;
      uint32_t CaseIdx = 0;
      const ComponentValType *PayloadTy = nullptr;
      bool Found = false;
      for (const auto &C : Cases) {
        if (C.first == *CaseName) {
          Found = true;
          if (C.second)
            PayloadTy = &*C.second;
          break;
        }
        CaseIdx++;
      }
      if (!Found)
        return std::nullopt;
      VariantVal V;
      V.Case = CaseIdx;
      V.Label = *CaseName;
      if (PayloadTy) {
        if (!consumeChar(Input, '('))
          return std::nullopt;
        auto Payload = parseComponentValue(Input, *PayloadTy, CompInst);
        if (!Payload)
          return std::nullopt;
        V.Payload = std::move(*Payload);
        if (!consumeChar(Input, ')'))
          return std::nullopt;
      }
      return makeComponentVal(std::move(V));
    } else if (DVT.isEnumTy()) {
      auto CaseName = parseIdentifier(Input);
      if (!CaseName)
        return std::nullopt;
      const auto &Cases = DVT.getEnum().Labels;
      uint32_t CaseIdx = 0;
      bool Found = false;
      for (const auto &C : Cases) {
        if (C == *CaseName) {
          Found = true;
          break;
        }
        CaseIdx++;
      }
      if (!Found)
        return std::nullopt;
      EnumVal E;
      E.Case = CaseIdx;
      E.Label = *CaseName;
      return makeComponentVal(std::move(E));
    } else if (DVT.isOptionTy()) {
      auto CaseName = parseIdentifier(Input);
      if (!CaseName)
        return std::nullopt;
      OptionVal O;
      if (*CaseName == "none") {
        return makeComponentVal(std::move(O));
      } else if (*CaseName == "some") {
        if (!consumeChar(Input, '('))
          return std::nullopt;
        auto Payload =
            parseComponentValue(Input, DVT.getOption().ValTy, CompInst);
        if (!Payload)
          return std::nullopt;
        O.Value = std::move(*Payload);
        if (!consumeChar(Input, ')'))
          return std::nullopt;
        return makeComponentVal(std::move(O));
      }
      return std::nullopt;
    } else if (DVT.isResultTy()) {
      auto CaseName = parseIdentifier(Input);
      if (!CaseName)
        return std::nullopt;
      ResultVal R;
      if (*CaseName == "ok") {
        R.IsOk = true;
        if (DVT.getResult().ValTy && consumeChar(Input, '(')) {
          auto Payload =
              parseComponentValue(Input, *DVT.getResult().ValTy, CompInst);
          if (!Payload)
            return std::nullopt;
          R.Payload = std::move(*Payload);
          if (!consumeChar(Input, ')'))
            return std::nullopt;
        }
        return makeComponentVal(std::move(R));
      } else if (*CaseName == "err") {
        R.IsOk = false;
        if (DVT.getResult().ErrTy && consumeChar(Input, '(')) {
          auto Payload =
              parseComponentValue(Input, *DVT.getResult().ErrTy, CompInst);
          if (!Payload)
            return std::nullopt;
          R.Payload = std::move(*Payload);
          if (!consumeChar(Input, ')'))
            return std::nullopt;
        }
        return makeComponentVal(std::move(R));
      }
      return std::nullopt;
    } else if (DVT.isFlagsTy()) {
      if (!consumeChar(Input, '{'))
        return std::nullopt;
      FlagsVal F;
      const auto &Labels = DVT.getFlags().Labels;
      F.Bits.resize(Labels.size(), false);
      while (!consumeChar(Input, '}')) {
        auto FlagName = parseIdentifier(Input);
        if (!FlagName)
          return std::nullopt;
        uint32_t Idx = 0;
        bool Found = false;
        for (const auto &L : Labels) {
          if (L == *FlagName) {
            F.Bits[Idx] = true;
            F.SetLabels.push_back(*FlagName);
            Found = true;
            break;
          }
          Idx++;
        }
        if (!Found)
          return std::nullopt;
        consumeChar(Input, ',');
      }
      return makeComponentVal(std::move(F));
    }
  }
  return std::nullopt;
}

inline void
printComponentValue(const ComponentValVariant &Val,
                    const ComponentValType &ValTy,
                    const Runtime::Instance::ComponentInstance *CompInst) {
  if (ValTy.isPrimValType()) {
    auto PrimTy = ValTy.getCode();
    switch (PrimTy) {
    case ComponentTypeCode::Bool:
      fmt::print("{}"sv, std::get<bool>(Val) ? "true" : "false");
      break;
    case ComponentTypeCode::S8:
      fmt::print("{}"sv, std::get<int8_t>(Val));
      break;
    case ComponentTypeCode::U8:
      fmt::print("{}"sv, std::get<uint8_t>(Val));
      break;
    case ComponentTypeCode::S16:
      fmt::print("{}"sv, std::get<int16_t>(Val));
      break;
    case ComponentTypeCode::U16:
      fmt::print("{}"sv, std::get<uint16_t>(Val));
      break;
    case ComponentTypeCode::S32:
      fmt::print("{}"sv, std::get<int32_t>(Val));
      break;
    case ComponentTypeCode::U32:
      fmt::print("{}"sv, std::get<uint32_t>(Val));
      break;
    case ComponentTypeCode::S64:
      fmt::print("{}"sv, std::get<int64_t>(Val));
      break;
    case ComponentTypeCode::U64:
      fmt::print("{}"sv, std::get<uint64_t>(Val));
      break;
    case ComponentTypeCode::F32:
      fmt::print("{}"sv, std::get<float>(Val));
      break;
    case ComponentTypeCode::F64:
      fmt::print("{}"sv, std::get<double>(Val));
      break;
    case ComponentTypeCode::Char:
      fmt::print("'{:c}'"sv, static_cast<char>(std::get<uint32_t>(Val)));
      break;
    case ComponentTypeCode::String:
      fmt::print("\"{}\""sv, std::get<std::string>(Val));
      break;
    default:
      fmt::print("<unsupported>"sv);
      break;
    }
  } else {
    // TypeIndex
    if (!CompInst)
      return;
    uint32_t TypeIdx = ValTy.getTypeIndex();
    const auto *DefTy = CompInst->getType(TypeIdx);
    if (!DefTy || !DefTy->isDefValType())
      return;
    const auto &DVT = DefTy->getDefValType();

    if (std::holds_alternative<std::shared_ptr<ValComp>>(Val)) {
      const auto &VC = std::get<std::shared_ptr<ValComp>>(Val);
      if (DVT.isListTy()) {
        const auto &L = std::get<ListVal>(VC->V);
        fmt::print("["sv);
        for (size_t I = 0; I < L.Elements.size(); ++I) {
          if (I > 0)
            fmt::print(", "sv);
          printComponentValue(L.Elements[I], DVT.getList().ValTy, CompInst);
        }
        fmt::print("]"sv);
      } else if (DVT.isRecordTy()) {
        const auto &R = std::get<RecordVal>(VC->V);
        fmt::print("{{"sv);
        for (size_t I = 0; I < R.Fields.size(); ++I) {
          if (I > 0)
            fmt::print(", "sv);
          fmt::print("{}: "sv, R.Fields[I].first);

          const ComponentValType *FieldTy = nullptr;
          for (const auto &F : DVT.getRecord().LabelTypes) {
            if (F.getLabel() == R.Fields[I].first) {
              FieldTy = &F.getValType();
              break;
            }
          }
          if (FieldTy) {
            printComponentValue(R.Fields[I].second, *FieldTy, CompInst);
          }
        }
        fmt::print("}}"sv);
      } else if (DVT.isTupleTy()) {
        const auto &T = std::get<TupleVal>(VC->V);
        fmt::print("("sv);
        for (size_t I = 0; I < T.Values.size(); ++I) {
          if (I > 0)
            fmt::print(", "sv);
          printComponentValue(T.Values[I], DVT.getTuple().Types[I], CompInst);
        }
        fmt::print(")"sv);
      } else if (DVT.isVariantTy()) {
        const auto &V = std::get<VariantVal>(VC->V);
        fmt::print("{}"sv, V.Label);
        if (V.Payload) {
          fmt::print("("sv);
          const ComponentValType *PayloadTy = nullptr;
          for (const auto &C : DVT.getVariant().Cases) {
            if (C.first == V.Label && C.second) {
              PayloadTy = &*C.second;
              break;
            }
          }
          if (PayloadTy) {
            printComponentValue(*V.Payload, *PayloadTy, CompInst);
          }
          fmt::print(")"sv);
        }
      } else if (DVT.isEnumTy()) {
        const auto &E = std::get<EnumVal>(VC->V);
        fmt::print("{}"sv, E.Label);
      } else if (DVT.isOptionTy()) {
        const auto &O = std::get<OptionVal>(VC->V);
        if (O.Value) {
          fmt::print("some("sv);
          printComponentValue(*O.Value, DVT.getOption().ValTy, CompInst);
          fmt::print(")"sv);
        } else {
          fmt::print("none"sv);
        }
      } else if (DVT.isResultTy()) {
        const auto &R = std::get<ResultVal>(VC->V);
        if (R.IsOk) {
          fmt::print("ok"sv);
          if (R.Payload && DVT.getResult().ValTy) {
            fmt::print("("sv);
            printComponentValue(*R.Payload, *DVT.getResult().ValTy, CompInst);
            fmt::print(")"sv);
          }
        } else {
          fmt::print("err"sv);
          if (R.Payload && DVT.getResult().ErrTy) {
            fmt::print("("sv);
            printComponentValue(*R.Payload, *DVT.getResult().ErrTy, CompInst);
            fmt::print(")"sv);
          }
        }
      } else if (DVT.isFlagsTy()) {
        const auto &F = std::get<FlagsVal>(VC->V);
        fmt::print("{{"sv);
        for (size_t I = 0; I < F.SetLabels.size(); ++I) {
          if (I > 0)
            fmt::print(", "sv);
          fmt::print("{}"sv, F.SetLabels[I]);
        }
        fmt::print("}}"sv);
      }
    }
  }
}

} // namespace Driver
} // namespace WasmEdge
