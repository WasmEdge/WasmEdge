// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadSection(AST::CoreInstanceSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(Sec, [this](AST::CoreInstance::T &Instance) {
      return loadCoreInstance(Instance);
    });
  });
}
Expect<void> Loader::loadCoreInstance(AST::CoreInstance::T &Instance) {
  // core:instance ::= ie:<instance-expr> => (instance ie)
  auto Res = FMgr.readByte();
  if (!Res.has_value()) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreInstance);
  }

  // core:instanceexpr ::=
  switch (Res.value()) {
  case 0x00: {
    // 0x00 m:<moduleidx> arg*:vec(<core:instantiatearg>)
    // => (instantiate m arg*)
    AST::CoreInstance::Instantiate &Inst =
        Instance.emplace<AST::CoreInstance::Instantiate>();
    if (auto ModIdx = FMgr.readU32(); ModIdx) {
      Inst.setModuleIdx(ModIdx.value());
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_CoreInstance);
    }
    return loadVec(Inst.getInstantiateArgs(),
                   [this](AST::CoreInstantiateArg &InstArg) -> Expect<void> {
                     auto Name = FMgr.readName();
                     if (!Name.has_value()) {
                       return logLoadError(Name.error(), FMgr.getLastOffset(),
                                           ASTNodeAttr::CompSec_CoreInstance);
                     }
                     auto B = FMgr.readByte();
                     if (!B.has_value()) {
                       return logLoadError(B.error(), FMgr.getLastOffset(),
                                           ASTNodeAttr::CompSec_CoreInstance);
                     }
                     auto Idx = FMgr.readU32();
                     if (!Idx.has_value()) {
                       return logLoadError(Idx.error(), FMgr.getLastOffset(),
                                           ASTNodeAttr::CompSec_CoreInstance);
                     }
                     InstArg.setName(Name.value());
                     InstArg.setIndex(Idx.value());

                     return {};
                   });
  }
  case 0x01:
    // 0x01 e*:vec(<core:export>) => e*
    {
      AST::CoreInstance::Export &Inst =
          Instance.emplace<AST::CoreInstance::Export>();
      return loadVec(Inst.getExports(),
                     [this](AST::ExportDecl &Export) -> Expect<void> {
                       // core:export ::= n:<name> si:<core:sortidx>
                       // => (export n si)
                       if (auto R = FMgr.readName(); R) {
                         Export.setName(*R);
                       } else {
                         return logLoadError(R.error(), FMgr.getLastOffset(),
                                             ASTNodeAttr::CompSec_Export);
                       }
                       return loadCoreSortIndex(Export.getExtern());
                     });
    }
  default:
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreInstance);
  }
}

Expect<void> Loader::loadSection(AST::CoreAliasSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::CoreAlias &Alias) { return loadCoreAlias(Alias); });
  });
}
Expect<void> Loader::loadCoreAlias(AST::CoreAlias &Alias) {
  // core:alias ::= sort:<core:sort> target:<core:aliastarget>
  //                => (core alias target (sort))
  if (auto Res = FMgr.readByte(); Res.has_value()) {
    Alias.setSort(Res.value());
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreAlias);
  }
  // core:aliastarget ::=
  //     0x00 i:<core:instanceidx> n:<name> => export i n
  //   | 0x01 ct:<u32> idx:<u32>            => outer ct idx
  auto Res = FMgr.readByte();
  if (!Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreAlias);
  }
  switch (Res.value()) {
  case 0x00: {
    uint32_t Idx;
    std::string_view Name;
    if (auto IR = FMgr.readU32(); IR.has_value()) {
      Idx = IR.value();
    } else {
      return logLoadError(IR.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_CoreAlias);
    }
    if (auto NR = FMgr.readName(); NR.has_value()) {
      Name = NR.value();
    } else {
      return logLoadError(NR.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_CoreAlias);
    }
    Alias.setTarget(AST::CoreAliasTarget::Export(Idx, Name));
    break;
  }
  case 0x01: {
    uint32_t C, I;
    if (auto CR = FMgr.readU32(); CR.has_value()) {
      C = CR.value();
    } else {
      return logLoadError(CR.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_CoreAlias);
    }
    if (auto IR = FMgr.readU32(); IR.has_value()) {
      I = IR.value();
    } else {
      return logLoadError(IR.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_CoreAlias);
    }
    Alias.setTarget(AST::CoreAliasTarget::Outer(C, I));
    break;
  }
  default:
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreAlias);
  }
  return {};
}

Expect<void> Loader::loadSection(AST::CoreTypeSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::CoreType &Ty) { return loadCoreType(Ty); });
  });
}
Expect<void> Loader::loadCoreType(AST::CoreType &Ty) {
  // core:type ::= dt:<core:deftype> => (type dt) (GC proposal)
  // core:deftype ::= ft:<core:functype>   => ft (WebAssembly 1.0)
  //                | st:<core:structtype> => st (GC proposal)
  //                | at:<core:arraytype>  => at (GC proposal)
  //                | mt:<core:moduletype> => mt
  auto Res = FMgr.readByte();
  if (!Res.has_value()) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreType);
  }

  switch (Res.value()) {
  case 0x60U: {
    AST::CoreDefType::FuncType FT;
    Ty = FT;
    return noJudgeLoadType(FT);
  }
  case 0x21U: {
    // core:structtype ::= 0x21 ft*:vec(fieldtype)
    // field ::= t:storagetype mut:mutability
    AST::CoreDefType::StructType ST;
    Ty = ST;
    return loadVec(ST.getFieldTypes(), [this](auto &FT) -> Expect<void> {
      return loadFieldType(FT);
    });
  }
  case 0x22U: {
    // core:arraytype ::= 0x22 ft:fieldtype
    // field ::= t:storagetype mut:mutability
    AST::CoreDefType::ArrayType AT;
    Ty = AT;
    return loadFieldType(AT.getField());
  }
  case 0x50U: {
    // core:moduletype ::= 0x50 md*:vec(<core:moduledecl>) => (module md*)
    AST::CoreDefType::ModuleType MT;
    Ty = MT;
    return loadVec(
        MT.getModuleDecls(), [this](AST::ModuleDecl &ModDecl) -> Expect<void> {
          // core:moduledecl ::= 0x00 i:<core:import>     => i
          //                   | 0x01 t:<core:type>       => t
          //                   | 0x02 a:<core:alias>      => a
          //                   | 0x03 e:<core:exportdecl> => e
          // core:exportdecl ::= n:<name> d:<core:importdesc> => (export n d)
          auto DeclType = FMgr.readByte();
          if (!DeclType.has_value()) {
            return logLoadError(DeclType.error(), FMgr.getLastOffset(),
                                ASTNodeAttr::CompSec_CoreType);
          }

          switch (DeclType.value()) {
          case 0x00: {
            // 0x00 i:<core:import>     => i
            AST::ImportDesc Desc;
            ModDecl = Desc;
            return loadDesc(Desc);
          }
          case 0x01: {
            // 0x01 t:<core:type>       => t
            AST::CoreType CT;
            ModDecl = CT;
            return loadCoreType(CT);
          }
          case 0x02: {
            // 0x02 a:<core:alias>      => a
            AST::CoreAlias Alias;
            ModDecl = Alias;
            return loadCoreAlias(Alias);
          }
          case 0x03: {
            // 0x03 e:<core:exportdecl> => e
            // core:exportdecl ::= n:<name> d:<core:importdesc> => (export n d)
            AST::ExportDesc Desc;
            ModDecl = Desc;
            return loadDesc(Desc);
          }
          default:
            return logLoadError(DeclType.error(), FMgr.getLastOffset(),
                                ASTNodeAttr::CompSec_CoreType);
          }
        });
  }
  default:
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreType);
  }
}
Expect<void> Loader::loadFieldType(AST::FieldType &Ty) {
  auto StorageTy = FMgr.readByte();
  if (!StorageTy.has_value()) {
    return logLoadError(StorageTy.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreType);
  }
  auto Mutability = FMgr.readByte();
  if (!Mutability.has_value()) {
    return logLoadError(Mutability.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreType);
  }
  switch (StorageTy.value()) {
  case 0x06:
    Ty = AST::FieldType(Mutability.value(), AST::FieldType::I8);
    break;
  case 0x07:
    Ty = AST::FieldType(Mutability.value(), AST::FieldType::I16);
    break;
  }
  return {};
}

Expect<void> Loader::loadSection(AST::InstanceSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(Sec,
                                 [this](AST::Instance &Inst) -> Expect<void> {
                                   return loadInstance(Inst);
                                 });
  });
}
Expect<void> Loader::loadInstance(AST::Instance &Inst) {
  // instance            ::= ie:<instance-expr> => (instance ie)
  auto Res = FMgr.readByte();
  if (!Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Instance);
  }

  // instanceexpr       ::= 0x00 c:<componentidx> arg*:vec(<instantiatearg>)
  //                      | 0x01 e*:vec(<export>)
  // instantiatearg     ::= n:<name> si:<sortidx> => (with n si)
  switch (*Res) {
  case 0x00: {
    // 0x00 c:<componentidx> arg*:vec(<instantiatearg>) => (instantiate c arg*)
    AST::InstanceExpr::Instantiate &Instantiate =
        Inst.emplace<AST::InstanceExpr::Instantiate>();
    auto Idx = FMgr.readU32();
    if (!Idx) {
      return logLoadError(Idx.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_Instance);
    }
    Instantiate.setIndex(*Idx);
    return loadVec(Instantiate.getArgs(),
                   [this](AST::InstantiateArg &Arg) -> Expect<void> {
                     auto Name = FMgr.readName();
                     if (!Name) {
                       return logLoadError(Name.error(), FMgr.getLastOffset(),
                                           ASTNodeAttr::CompSec_Instance);
                     }
                     Arg.setName(*Name);
                     return loadSortIndex(Arg.getSortIndex());
                   });
  }
  case 0x01: {
    // 0x01 e*:vec(<export>) => e*
    AST::InstanceExpr::Export &Export =
        Inst.emplace<AST::InstanceExpr::Export>();
    return loadVec(Export.getExports(),
                   [this](AST::ExportDecl &ExpDecl) -> Expect<void> {
                     return loadExportDecl(ExpDecl);
                   });
  }
  default:
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Instance);
  }
}

Expect<void> Loader::loadSection(AST::AliasSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec,
        [this](AST::Alias &Alias) -> Expect<void> { return loadAlias(Alias); });
  });
}
Expect<void> Loader::loadAlias(AST::Alias &Alias) {
  // alias       ::= s:<sort> t:<aliastarget> => (alias t (s))
  loadSort(Alias.getSort());

  // aliastarget ::= 0x00 i:<instanceidx> n:<name>    => export i n
  //               | 0x01 i:<core:instanceidx> n:<name> => core export i n
  //               | 0x02 ct:<u32> idx:<u32>          => outer ct idx
  auto Res = FMgr.readByte();
  if (!Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Alias);
  }
  switch (*Res) {
  case 0x00: {
    // 0x00 i:<instanceidx> n:<name>    => export i n
    auto Idx = FMgr.readU32();
    if (!Idx) {
      return logLoadError(Idx.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_Alias);
    }
    auto Name = FMgr.readName();
    if (!Name) {
      return logLoadError(Name.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_Alias);
    }
    Alias.getTarget() = AST::AliasTarget::Export(*Idx, *Name);
    break;
  }
  case 0x01: {
    // 0x01 i:<core:instanceidx> n:<name> => core export i n
    auto Idx = FMgr.readU32();
    if (!Idx) {
      return logLoadError(Idx.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_Alias);
    }
    auto Name = FMgr.readName();
    if (!Name) {
      return logLoadError(Name.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_Alias);
    }
    Alias.getTarget() = AST::AliasTarget::CoreExport(*Idx, *Name);
    break;
  }
  case 0x02: {
    // 0x02 ct:<u32> idx:<u32>          => outer ct idx
    auto Ct = FMgr.readU32();
    if (!Ct) {
      return logLoadError(Ct.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_Alias);
    }
    auto Idx = FMgr.readU32();
    if (!Idx) {
      return logLoadError(Idx.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_Alias);
    }
    Alias.getTarget() = AST::AliasTarget::Outer(*Ct, *Idx);
    break;
  }
  default:
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Alias);
  }
  return {};
}

Expect<void> Loader::loadSection(AST::ComponentTypeSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Type &Ty) -> Expect<void> { return loadType(Ty); });
  });
}
Expect<void> Loader::loadType(AST::Type &Ty) {
  auto Res = FMgr.readByte();
  if (!Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
  switch (*Res) {
  case 0x40: {
    // => (func (param p)* (result r)*)
    // functype ::= 0x40 p*:<funcvec> r*:<funcvec>
    AST::FuncType &FuncTy = Ty.getData().emplace<AST::FuncType>();
    auto R = loadFuncVec(FuncTy.getParameters());
    if (!R) {
      return R;
    }
    return loadFuncVec(FuncTy.getReturns());
  }
  case 0x41: {
    // componenttype ::= 0x41 cd*:vec(<componentdecl>) => (component cd*)
    AST::ComponentType &CompTy = Ty.getData().emplace<AST::ComponentType>();
    return loadVec(CompTy.getDecls(), [this](auto &D) -> Expect<void> {
      return loadComponentDecl(D);
    });
  }
  case 0x42: {
    // instancetype  ::= 0x42 id*:vec(<instancedecl>) => (instance id*)
    AST::InstanceType &InstTy = Ty.getData().emplace<AST::InstanceType>();
    return loadVec(InstTy.getDecls(), [this](auto &D) -> Expect<void> {
      return loadInstanceDecl(D);
    });
  }
  case static_cast<Byte>(AST::PrimitiveValueType::String):
  case static_cast<Byte>(AST::PrimitiveValueType::Char):
  case static_cast<Byte>(AST::PrimitiveValueType::Float64):
  case static_cast<Byte>(AST::PrimitiveValueType::Float32):
  case static_cast<Byte>(AST::PrimitiveValueType::U64):
  case static_cast<Byte>(AST::PrimitiveValueType::S64):
  case static_cast<Byte>(AST::PrimitiveValueType::U32):
  case static_cast<Byte>(AST::PrimitiveValueType::S32):
  case static_cast<Byte>(AST::PrimitiveValueType::U16):
  case static_cast<Byte>(AST::PrimitiveValueType::S16):
  case static_cast<Byte>(AST::PrimitiveValueType::U8):
  case static_cast<Byte>(AST::PrimitiveValueType::S8):
  case static_cast<Byte>(AST::PrimitiveValueType::Bool):
    Ty.getData().emplace<AST::PrimitiveValueType>(
        static_cast<AST::PrimitiveValueType>(*Res));
    return {};

  case 0x72: {
    // 0x72 nt*:vec(<namedvaltype>)         => (record (field nt)*)
    AST::DefinedValueType::Record &RecordTy =
        Ty.getData().emplace<AST::DefinedValueType::Record>();
    return loadVec(RecordTy.getFields(), [this](auto &T) -> Expect<void> {
      return loadNamedValType(T);
    });
  }
  case 0x71: {
    // 0x71 case*:vec(<case>)               => (variant case*)
    AST::DefinedValueType::Variant &VariantTy =
        Ty.getData().emplace<AST::DefinedValueType::Variant>();
    return loadVec(VariantTy.getCases(), [this](auto &Case) -> Expect<void> {
      return loadCase(Case);
    });
  }
  case 0x70: {
    // 0x70 t:<valtype>                     => (list t)
    AST::DefinedValueType::List &ListTy =
        Ty.getData().emplace<AST::DefinedValueType::List>();
    return loadValType(ListTy.getType());
  }
  case 0x6f: {
    // 0x6f t*:vec(<valtype>)               => (tuple t*)
    AST::DefinedValueType::Tuple &TupleTy =
        Ty.getData().emplace<AST::DefinedValueType::Tuple>();
    return loadVec(TupleTy.getTypes(),
                   [this](auto &T) { return loadValType(T); });
  }
  case 0x6e: {
    // 0x6e n*:vec(<name>)                  => (flags n*)
    AST::DefinedValueType::Flags &F =
        Ty.getData().emplace<AST::DefinedValueType::Flags>();
    return loadVec(F.getNames(), [this](auto &S) -> Expect<void> {
      auto N = FMgr.readName();
      if (!N) {
        return logLoadError(N.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::CompSec_Type);
      }
      S = *N;
      return {};
    });
  }
  case 0x6d: {
    // 0x6d n*:vec(<name>)                  => (enum n*)
    AST::DefinedValueType::Enum &E =
        Ty.getData().emplace<AST::DefinedValueType::Enum>();
    return loadVec(E.getNames(), [this](auto &S) -> Expect<void> {
      auto N = FMgr.readName();
      if (!N) {
        return logLoadError(N.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::CompSec_Type);
      }
      S = *N;
      return {};
    });
  }
  case 0x6c: {
    // 0x6c t*:vec(<valtype>)               => (union t*)
    AST::DefinedValueType::Union &UnionTy =
        Ty.getData().emplace<AST::DefinedValueType::Union>();
    return loadVec(UnionTy.getTypes(),
                   [this](auto &T) { return loadValType(T); });
  }
  case 0x6b: {
    // 0x6b t:<valtype>                     => (option t)
    AST::DefinedValueType::Option &OptTy =
        Ty.getData().emplace<AST::DefinedValueType::Option>();
    return loadValType(OptTy.getType());
  }
  case 0x6a: {
    // 0x6a t?:<casetype> u?:<casetype>     => (result t? (error u)?)
    AST::DefinedValueType::Result ResultTy =
        Ty.getData().emplace<AST::DefinedValueType::Result>();
    if (auto E = loadCaseType(ResultTy.getResult()); !E) {
      return E;
    }
    return loadCaseType(ResultTy.getError());
  }
  default:
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
}
Expect<void> Loader::loadInstanceDecl(AST::InstanceDecl &Decl, Expect<Byte> B) {
  // instancedecl  ::= 0x00 t:<core:type>                   => t
  //                 | 0x01 t:<type>                        => t
  //                 | 0x02 a:<alias>                       => a
  //                 | 0x04 ed:<exportdecl>                 => ed
  switch (*B) {
  case 0x00:
    return loadCoreType(Decl.emplace<AST::CoreType>());
  case 0x01: {
    auto Ty = std::make_shared<AST::Type>();
    Decl = Ty;
    return loadType(*Ty);
  }
  case 0x02:
    return loadAlias(Decl.emplace<AST::Alias>());
  case 0x04:
    return loadExportDecl(Decl.emplace<AST::ExportDecl>());
  default:
    return logLoadError(B.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_InstanceDecl);
  }
}
Expect<void> Loader::loadInstanceDecl(AST::InstanceDecl &Decl) {
  // instancedecl  ::= 0x00 t:<core:type>                   => t
  //                 | 0x01 t:<type>                        => t
  //                 | 0x02 a:<alias>                       => a
  //                 | 0x04 ed:<exportdecl>                 => ed
  auto B = FMgr.readByte();
  if (!B) {
    return logLoadError(B.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_InstanceDecl);
  }
  return loadInstanceDecl(Decl, B);
}
Expect<void> Loader::loadComponentDecl(AST::ComponentDecl &Decl) {
  // componentdecl ::= 0x03 id:<importdecl>                 => id
  //                 | id:<instancedecl>                    => id
  auto B = FMgr.readByte();
  if (!B) {
    return logLoadError(B.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_ComponentDecl);
  }
  switch (*B) {
  case 0x03:
    Decl.emplace<AST::ImportDecl>();
    return loadImportDecl(std::get<AST::ImportDecl>(Decl));
  default:
    Decl.emplace<AST::InstanceDecl>();
    return loadInstanceDecl(std::get<AST::InstanceDecl>(Decl), B);
  }
}
Expect<void> Loader::loadFuncVec(AST::FuncVec &FuncV) {
  auto R = FMgr.readByte();
  if (!R) {
    return logLoadError(R.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
  switch (*R) {
  case 0x00:
    FuncV.emplace<AST::ValueType>();
    return loadValType(std::get<AST::ValueType>(FuncV));
  case 0x01:
    FuncV.emplace<std::vector<AST::NamedValType>>();
    return loadVec(
        std::get<std::vector<AST::NamedValType>>(FuncV),
        [this](auto &T) -> Expect<void> { return loadNamedValType(T); });
  default:
    return logLoadError(R.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
}
Expect<void> Loader::loadCase(AST::Case &Case) {
  if (auto N = FMgr.readName(); N) {
    Case.setName(*N);
  } else {
    return logLoadError(N.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }

  if (auto E = loadCaseType(Case.getType()); !E) {
    return E;
  }

  auto B = FMgr.readByte();
  if (!B) {
    return logLoadError(B.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
  switch (*B) {
  case 0x00:
    break;
  case 0x01:
    if (auto R = FMgr.readU32(); R) {
      Case.setLabelIndex(*R);
    } else {
      return logLoadError(R.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_Type);
    }
    break;
  default:
    return logLoadError(B.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
  return {};
}
Expect<void> Loader::loadCaseType(std::optional<AST::CaseType> Ty) {
  auto R = FMgr.readByte();
  if (!R) {
    return logLoadError(R.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
  switch (*R) {
  case 0x00:
    // Do nothing, in this case, it's the parameter type of CaseType is nullopt
    return {};
  case 0x01: {
    AST::ValueType VT;
    Ty->getType() = VT;
    return loadValType(VT);
  }
  default:
    return logLoadError(R.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
}
Expect<void> Loader::loadNamedValType(AST::NamedValType &Ty) {
  if (auto Res = FMgr.readName(); Res) {
    Ty.setName(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
  return loadValType(Ty.getType());
}
Expect<void> Loader::loadValType(AST::ValueType &Ty) {
  auto Res = FMgr.readByte();
  if (!Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Type);
  }
  switch (*Res) {
  case static_cast<Byte>(AST::PrimitiveValueType::String):
  case static_cast<Byte>(AST::PrimitiveValueType::Char):
  case static_cast<Byte>(AST::PrimitiveValueType::Float64):
  case static_cast<Byte>(AST::PrimitiveValueType::Float32):
  case static_cast<Byte>(AST::PrimitiveValueType::U64):
  case static_cast<Byte>(AST::PrimitiveValueType::S64):
  case static_cast<Byte>(AST::PrimitiveValueType::U32):
  case static_cast<Byte>(AST::PrimitiveValueType::S32):
  case static_cast<Byte>(AST::PrimitiveValueType::U16):
  case static_cast<Byte>(AST::PrimitiveValueType::S16):
  case static_cast<Byte>(AST::PrimitiveValueType::U8):
  case static_cast<Byte>(AST::PrimitiveValueType::S8):
  case static_cast<Byte>(AST::PrimitiveValueType::Bool): {
    Ty = static_cast<AST::PrimitiveValueType>(*Res);
    break;
  }
  default:
    Ty = static_cast<uint32_t>(*Res);
    break;
  }
  return {};
}

Expect<void> Loader::loadSection(AST::ComponentCanonSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(
        Sec, [this](AST::Canon &Canon) { return loadCanon(Canon); });
  });
}
Expect<void> Loader::loadCanon(AST::Canon &Canon) {
  if (auto DirectionByte = FMgr.readByte(); DirectionByte.has_value()) {
    switch (DirectionByte.value()) {
    case 0x00: {
      // 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
      // => (canon lift f opts type-index-space[ft])
      AST::Lift &CanonLift = Canon.emplace<AST::Lift>();
      if (auto Res = FMgr.readByte(); !Res.has_value() || Res.value() != 0x00) {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::CompSec_Canon);
      }
      // f:<core:funcidx>
      auto CoreFuncIdx = FMgr.readU32();
      if (!CoreFuncIdx.has_value()) {
        return logLoadError(CoreFuncIdx.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::CompSec_Canon);
      }
      CanonLift.setCoreFuncIdx(CoreFuncIdx.value());

      // opts ::= opt*:vec(<canonopt>) => opt*
      auto Res = loadVec(CanonLift.getOpts(),
                         [this](AST::CanonOpt &CanonOpt) -> Expect<void> {
                           return loadCanonOpt(CanonOpt);
                         });
      if (!Res.has_value()) {
        return Res;
      }

      // ft:<typeidx>
      auto TypeIdx = FMgr.readByte();
      if (!TypeIdx.has_value()) {
        return logLoadError(TypeIdx.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::CompSec_Canon);
      }
      CanonLift.setTypeIdx(TypeIdx.value());
      break;
    }
    case 0x01: {
      // 0x01 0x00 f:<funcidx> opts:<opts>
      // => (canon lower f opts (core func))
      AST::Lower &CanonLower = Canon.emplace<AST::Lower>();
      if (auto Res = FMgr.readByte(); !Res.has_value() || Res.value() != 0x00) {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::CompSec_Canon);
      }
      auto FuncIdx = FMgr.readU32();
      if (!FuncIdx.has_value()) {
        return logLoadError(FuncIdx.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::CompSec_Canon);
      }
      CanonLower.setFuncIdx(FuncIdx.value());

      // opts ::= opt*:vec(<canonopt>) => opt*
      return loadVec(CanonLower.getOpts(),
                     [this](AST::CanonOpt &CanonOpt) -> Expect<void> {
                       return loadCanonOpt(CanonOpt);
                     });
    }
    default:
      return logLoadError(DirectionByte.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_Canon);
    }

    return {};
  } else {
    return logLoadError(DirectionByte.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Canon);
  }
}
Expect<void> Loader::loadCanonOpt(AST::CanonOpt &CanonOpt) {
  auto CanonOptType = FMgr.readByte();
  if (!CanonOptType.has_value()) {
    return logLoadError(CanonOptType.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Canon);
  }
  switch (CanonOptType.value()) {
  case 0x00: {
    // 0x00 => string-encoding=utf8
    CanonOpt.emplace<AST::StringEncodingUTF8>();
    break;
  }
  case 0x01: {
    // 0x01 => string-encoding=utf16
    CanonOpt.emplace<AST::StringEncodingUTF16>();
    break;
  }
  case 0x02: {
    // 0x02 => string-encoding=latin1+utf16
    CanonOpt.emplace<AST::StringEncodingLatin1UTF16>();
    break;
  }
  case 0x03: {
    // 0x03 m:<core:memidx> => (memory m)
    auto Res = FMgr.readU32();
    if (!Res.has_value()) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_Canon);
    }
    CanonOpt.emplace<AST::MemoryIndex>(*Res);
    break;
  }
  case 0x04: {
    // 0x04 f:<core:funcidx> => (realloc f)
    auto Res = FMgr.readU32();
    if (!Res.has_value()) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_Canon);
    }
    CanonOpt.emplace<AST::ReallocFunc>(*Res);
    break;
  }
  case 0x05: {
    // 0x05 f:<core:funcidx> => (post-return f)
    auto Res = FMgr.readU32();
    if (!Res.has_value()) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::CompSec_Canon);
    }
    CanonOpt.emplace<AST::PostReturnFunc>(*Res);
    break;
  }
  default:
    return logLoadError(CanonOptType.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Canon);
  }
  return {};
}

Expect<void> Loader::loadStart(AST::Start &Start) {
  using AST::ValueIdx;
  // start ::= f:<funcidx> arg*:vec(<valueidx>)
  auto FuncIdx = FMgr.readU32();
  if (!FuncIdx.has_value()) {
    return logLoadError(FuncIdx.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Start);
  }
  Start.setFuncIdx(FuncIdx.value());

  if (auto Res = loadVec(Start.getArgs(),
                         [this](ValueIdx &ValueIdx) -> Expect<void> {
                           auto Idx = FMgr.readU32();
                           if (!Idx.has_value()) {
                             return logLoadError(Idx.error(),
                                                 FMgr.getLastOffset(),
                                                 ASTNodeAttr::CompSec_Start);
                           }
                           ValueIdx = *Idx;
                           return {};
                         });
      !Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Start);
  }
  return {};
}

Expect<void> Loader::loadSection(AST::ComponentImportSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(Sec, [this](AST::ImportDecl &Import) {
      return loadImportDecl(Import);
    });
  });
}
Expect<void> Loader::loadImportDecl(AST::ImportDecl &Import) {
  if (auto Res = FMgr.readName(); Res.has_value()) {
    Import.setName(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_ImportDecl);
  }

  auto B = FMgr.readByte();
  if (!B) {
    return logLoadError(B.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_ImportDecl);
  }
  switch (*B) {
  case 0x00: {
    // 0x00 0x11 i:<core:typeidx>           => (core module (type i))
    if (auto Res = FMgr.readByte(); !Res.has_value() || Res.value() != 0x11) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_ImportDecl);
    }
    auto TypeIdx = FMgr.readU32();
    if (!TypeIdx.has_value()) {
      return logLoadError(TypeIdx.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_ImportDecl);
    }
    Import.getExtern().emplace<AST::ExternDesc::CoreType>(TypeIdx.value());
    return {};
  }
  case 0x01: {
    // 0x01 i:<typeidx>                     => (func (type i))
    auto TypeIdx = FMgr.readU32();
    if (!TypeIdx.has_value()) {
      return logLoadError(TypeIdx.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_ImportDecl);
    }
    Import.getExtern().emplace<AST::ExternDesc::FuncType>(TypeIdx.value());
    return {};
  }
  case 0x02: {
    // 0x02 t:<valtype>                     => (value t)
    return loadValType(Import.getExtern().emplace<AST::ValueType>());
  }
  case 0x03: {
    // 0x03 b:<typebound>                   => (type b)
    if (auto Res = FMgr.readByte(); !Res.has_value() || Res.value() != 0x00) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_ImportDecl);
    }
    auto TypeIdx = FMgr.readU32();
    if (!TypeIdx.has_value()) {
      return logLoadError(TypeIdx.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_ImportDecl);
    }
    Import.getExtern().emplace<AST::ExternDesc::TypeBound>(TypeIdx.value());
    return {};
  }
  case 0x04: {
    // 0x04 i:<typeidx>                     => (instance (type i))
    auto TypeIdx = FMgr.readU32();
    if (!TypeIdx.has_value()) {
      return logLoadError(TypeIdx.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_ImportDecl);
    }
    Import.getExtern().emplace<AST::ExternDesc::InstanceType>(TypeIdx.value());
    return {};
  }
  case 0x05: {
    // 0x05 i:<typeidx>                     => (component (type i))
    auto TypeIdx = FMgr.readU32();
    if (!TypeIdx.has_value()) {
      return logLoadError(TypeIdx.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_ImportDecl);
    }
    Import.getExtern().emplace<AST::ExternDesc::ComponentType>(TypeIdx.value());
    return {};
  }
  default:
    return logLoadError(B.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_ImportDecl);
  }
}

Expect<void> Loader::loadSection(AST::ComponentExportSection &Sec) {
  return loadSectionContent(Sec, [this, &Sec]() {
    return loadSectionContentVec(Sec, [this](AST::ExportDecl &Export) {
      return loadExportDecl(Export);
    });
  });
}
Expect<void> Loader::loadExportDecl(AST::ExportDecl &Export) {
  // export ::= n:<name> si:<sortidx>
  // n:<name>
  if (auto Res = FMgr.readName(); Res.has_value()) {
    Export.setName(*Res);
    return loadSortIndex(Export.getExtern());
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_ExportDecl);
  }
}

Expect<void> Loader::loadSortIndex(AST::SortIndex &SortIdx) {
  // sortidx             ::= sort:<sort> idx:<u32> => (sort idx)
  if (auto R = loadSort(SortIdx.getSort()); !R) {
    return logLoadError(R.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Export);
  }
  auto Idx = FMgr.readU32();
  if (!Idx) {
    return logLoadError(Idx.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_Export);
  }
  SortIdx.setIndex(*Idx);
  return {};
}

Expect<void> Loader::loadCoreSortIndex(AST::SortIndex &SortIdx) {
  // core:sortidx ::= sort:<core:sort> idx:<u32> => (sort idx)
  if (auto R = loadCoreSort(SortIdx.getSort()); !R) {
    return logLoadError(R.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreInstance);
  }
  auto Idx = FMgr.readU32();
  if (!Idx) {
    return logLoadError(Idx.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::CompSec_CoreInstance);
  }
  SortIdx.setIndex(*Idx);
  return {};
}

Expect<void> Loader::loadSort(AST::Sort &Sort) {
  // sort                ::= 0x00 cs:<core:sort> => core cs
  //                       | 0x01 => func
  //                       | 0x02 => value
  //                       | 0x03 => type
  //                       | 0x04 => component
  //                       | 0x05 => instance
  // si:<sortidx>
  auto Res = FMgr.readByte();
  if (!Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Sort);
  }
  switch (*Res) {
  case 0x00:
    return loadCoreSort(Sort);
  case 0x01:
    Sort = AST::Sort::Func;
    return {};
  case 0x02:
    Sort = AST::Sort::Value;
    return {};
  case 0x03:
    Sort = AST::Sort::Type;
    return {};
  case 0x04:
    Sort = AST::Sort::Component;
    return {};
  case 0x05:
    Sort = AST::Sort::Instance;
    return {};
  default:
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Sort);
  }
}
Expect<void> Loader::loadCoreSort(AST::Sort &Sort) {
  // core:sort ::= 0x00 => func
  //             | 0x01 => table
  //             | 0x02 => memory
  //             | 0x03 => global
  //             | 0x10 => type
  //             | 0x11 => module
  //             | 0x12 => instance
  auto Res = FMgr.readByte();
  if (!Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreSort);
  }
  switch (*Res) {
  case 0x00:
    Sort = AST::Sort::CoreFunc;
    return {};
  case 0x01:
    Sort = AST::Sort::Table;
    return {};
  case 0x02:
    Sort = AST::Sort::Memory;
    return {};
  case 0x03:
    Sort = AST::Sort::Global;
    return {};
  case 0x10:
    Sort = AST::Sort::CoreType;
    return {};
  case 0x11:
    Sort = AST::Sort::Module;
    return {};
  case 0x12:
    Sort = AST::Sort::CoreInstance;
    return {};
  default:
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreSort);
  }
}

} // namespace Loader
} // namespace WasmEdge
