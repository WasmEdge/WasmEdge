// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

use heck::ShoutySnakeCase;
use witx::*;

pub fn to_cpp_header(doc: &Document, inputs_str: &str) -> String {
    let mut ret = String::new();

    ret.push_str(&format!(
        r#"// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

/**
 * THIS FILE IS AUTO-GENERATED from the following files:
 *   {}
 *
 * @file
 * This file describes the [WASI] interface, consisting of functions, types,
 * and defined values (macros).
 *
 * The interface described here is greatly inspired by [CloudABI]'s clean,
 * thoughtfully-designed, capability-oriented, POSIX-style API.
 *
 * [CloudABI]: https://github.com/NuxiNL/cloudlibc
 * [WASI]: https://github.com/WebAssembly/WASI/
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

using const_uint8_t_ptr = uint32_t;
using uint8_t_ptr = uint32_t;

#define DEFINE_ENUM_OPERATORS(type)                                            \
  inline constexpr type operator~(type a) noexcept {{                           \
    return static_cast<type>(~static_cast<std::underlying_type_t<type>>(a));   \
  }}                                                                            \
  inline constexpr type operator|(type a, type b) noexcept {{                   \
    return static_cast<type>(static_cast<std::underlying_type_t<type>>(a) |    \
                             static_cast<std::underlying_type_t<type>>(b));    \
  }}                                                                            \
  inline constexpr type &operator|=(type &a, type b) noexcept {{                \
    a = a | b;                                                                 \
    return a;                                                                  \
  }}                                                                            \
  inline constexpr type operator&(type a, type b) noexcept {{                   \
    return static_cast<type>(static_cast<std::underlying_type_t<type>>(a) &    \
                             static_cast<std::underlying_type_t<type>>(b));    \
  }}                                                                            \
  inline constexpr type &operator&=(type &a, type b) noexcept {{                \
    a = a & b;                                                                 \
    return a;                                                                  \
  }}

static_assert(alignof(int8_t) == 1, "non-wasi data layout");
static_assert(alignof(uint8_t) == 1, "non-wasi data layout");
static_assert(alignof(int16_t) == 2, "non-wasi data layout");
static_assert(alignof(uint16_t) == 2, "non-wasi data layout");
static_assert(alignof(int32_t) == 4, "non-wasi data layout");
static_assert(alignof(uint32_t) == 4, "non-wasi data layout");
static_assert(alignof(int64_t) == 8, "non-wasi data layout");
static_assert(alignof(uint64_t) == 8, "non-wasi data layout");
static_assert(alignof(const_uint8_t_ptr) == 4, "non-wasi data layout");
static_assert(alignof(uint8_t_ptr) == 4, "non-wasi data layout");

"#,
        inputs_str,
    ));

    for nt in doc.typenames() {
        print_datatype(&mut ret, &*nt);
    }

    ret
}

fn print_datatype(ret: &mut String, nt: &NamedType) {
    if !nt.docs.is_empty() {
        ret.push_str("/**\n");
        for line in nt.docs.lines() {
            ret.push_str(&format!(" * {}\n", line));
        }
        ret.push_str(" */\n");
    }

    match &nt.tref {
        TypeRef::Value(v) => match &**v {
            Type::Record(s) => print_record(ret, &nt.name, s),
            Type::Variant(u) => print_variant(ret, &nt.name, u),
            Type::Handle(h) => print_handle(ret, &nt.name, h),
            Type::Builtin { .. }
            | Type::List { .. }
            | Type::Pointer { .. }
            | Type::ConstPointer { .. } => print_alias(ret, &nt.name, &nt.tref),
        },
        TypeRef::Name(_) => print_alias(ret, &nt.name, &nt.tref),
    }
}

fn print_alias(ret: &mut String, name: &Id, dest: &TypeRef) {
    match &**dest.type_() {
        Type::List(_) => {
            // Don't emit arrays as top-level types; instead we special-case
            // them in places like parameter lists so that we can pass them
            // as pointer and length pairs.
        }
        _ => {
            ret.push_str(&format!(
                "using __wasi_{}_t = {};\n",
                ident_name(name),
                typeref_name(dest)
            ));
            ret.push_str("\n");

            ret.push_str(&format!(
                "static_assert(sizeof(__wasi_{}_t) == {}, \"witx calculated size\");\n",
                ident_name(name),
                dest.mem_size_align().size
            ));
            ret.push_str(&format!(
                "static_assert(alignof(__wasi_{}_t) == {}, \"witx calculated align\");\n",
                ident_name(name),
                dest.mem_size_align().align
            ));

            ret.push_str("\n");
        }
    }
}

fn print_enum(ret: &mut String, name: &Id, v: &Variant) {
    ret.push_str(&format!(
        "enum __wasi_{}_e_t : {} {{\n",
        ident_name(name),
        intrepr_name(v.tag_repr)
    ));

    for (index, case) in v.cases.iter().enumerate() {
        if !case.docs.is_empty() {
            ret.push_str("  /**\n");
            for line in case.docs.lines() {
                ret.push_str(&format!("   * {}\n", line));
            }
            ret.push_str("   */\n");
        }
        ret.push_str(&format!(
            "  __WASI_{}_{} = {},\n",
            ident_name(&name).to_shouty_snake_case(),
            ident_name(&case.name).to_shouty_snake_case(),
            index
        ));
        ret.push_str("\n");
    }
    ret.push_str("};\n");

    ret.push_str(&format!(
        "static_assert(sizeof(__wasi_{}_e_t) == {}, \"witx calculated size\");\n",
        ident_name(name),
        v.tag_repr.mem_size()
    ));
    ret.push_str(&format!(
        "static_assert(alignof(__wasi_{}_e_t) == {}, \"witx calculated align\");\n",
        ident_name(name),
        v.tag_repr.mem_align()
    ));

    ret.push_str("\n");
}

fn print_record(ret: &mut String, name: &Id, s: &RecordDatatype) {
    if let Some(repr) = s.bitflags_repr() {
        ret.push_str(&format!(
            "enum __wasi_{}_t : {} {{\n",
            ident_name(name),
            intrepr_name(repr)
        ));
        ret.push_str("\n");
        for (i, member) in s.members.iter().enumerate() {
            if !member.docs.is_empty() {
                ret.push_str("  /**\n");
                for line in member.docs.lines() {
                    ret.push_str(&format!("   * {}\n", line));
                }
                ret.push_str("   */\n");
            }
            ret.push_str(&format!(
                "  __WASI_{}_{} = 1ULL << {},\n",
                ident_name(name).to_shouty_snake_case(),
                ident_name(&member.name).to_shouty_snake_case(),
                i,
            ));
            ret.push_str("\n");
        }
        ret.push_str(&format!(
            "}};\nDEFINE_ENUM_OPERATORS(__wasi_{}_t)\n\n",
            ident_name(name)
        ));
        ret.push_str(&format!(
            "static_assert(sizeof(__wasi_{}_t) == {}, \"witx calculated size\");\n",
            ident_name(name),
            repr.mem_size()
        ));
        ret.push_str(&format!(
            "static_assert(alignof(__wasi_{}_t) == {}, \"witx calculated align\");\n",
            ident_name(name),
            repr.mem_align()
        ));
        ret.push_str("\n");
        return;
    }

    ret.push_str(&format!("struct __wasi_{}_t {{\n", ident_name(name)));

    for member in &s.members {
        if !member.docs.is_empty() {
            ret.push_str("  /**\n");
            for line in member.docs.lines() {
                ret.push_str(&format!("   * {}\n", line));
            }
            ret.push_str("   */\n");
        }
        ret.push_str(&format!(
            "  {} {};\n",
            typeref_name(&member.tref),
            ident_name(&member.name)
        ));
        ret.push_str("\n");
    }

    ret.push_str(&format!("}};\n"));
    ret.push_str("\n");

    ret.push_str(&format!(
        "static_assert(sizeof(__wasi_{}_t) == {}, \"witx calculated size\");\n",
        ident_name(name),
        s.mem_size()
    ));
    ret.push_str(&format!(
        "static_assert(alignof(__wasi_{}_t) == {}, \"witx calculated align\");\n",
        ident_name(name),
        s.mem_align()
    ));

    for layout in s.member_layout() {
        ret.push_str(&format!(
            "static_assert(offsetof(__wasi_{}_t, {}) == {}, \"witx calculated offset\");\n",
            ident_name(name),
            ident_name(&layout.member.name),
            layout.offset
        ));
    }

    ret.push_str("\n");
}

fn print_variant(ret: &mut String, name: &Id, v: &Variant) {
    if v.is_enum() {
        return print_enum(ret, name, v);
    }

    ret.push_str(&format!("union __wasi_{}_u_t {{\n", ident_name(name)));

    for case in &v.cases {
        if let Some(tref) = &case.tref {
            if !case.docs.is_empty() {
                ret.push_str("  /**\n");
                for line in case.docs.lines() {
                    ret.push_str(&format!("  * {}\n", line));
                }
                ret.push_str("  */\n");
            }
            ret.push_str(&format!(
                "  {} {};\n",
                typeref_name(tref),
                ident_name(&case.name)
            ));
        }
    }
    ret.push_str("};\n");

    ret.push_str(&format!("struct __wasi_{}_t {{\n", ident_name(name)));
    ret.push_str(&format!("  {} tag;\n", intrepr_name(v.tag_repr)));
    ret.push_str(&format!("  __wasi_{}_u_t u;\n", ident_name(name)));
    ret.push_str("};\n\n");

    ret.push_str(&format!(
        "static_assert(sizeof(__wasi_{}_t) == {}, \"witx calculated size\");\n",
        ident_name(name),
        v.mem_size()
    ));
    ret.push_str(&format!(
        "static_assert(alignof(__wasi_{}_t) == {}, \"witx calculated align\");\n",
        ident_name(name),
        v.mem_align()
    ));
    ret.push_str(&format!(
        "static_assert(offsetof(__wasi_{}_t, u) == {}, \"witx calculated union offset\");\n",
        ident_name(name),
        v.payload_offset()
    ));

    ret.push_str("\n");
}

fn print_handle(ret: &mut String, name: &Id, h: &HandleDatatype) {
    ret.push_str(&format!(
        "using __wasi_{}_t = int32_t;\n\n",
        ident_name(name)
    ));

    ret.push_str(&format!(
        "static_assert(sizeof(__wasi_{}_t) == {}, \"witx calculated size\");\n",
        ident_name(name),
        h.mem_size()
    ));
    ret.push_str(&format!(
        "static_assert(alignof(__wasi_{}_t) == {}, \"witx calculated align\");\n",
        ident_name(name),
        h.mem_align()
    ));

    ret.push_str("\n");
}

fn ident_name(i: &Id) -> String {
    i.as_str().to_string()
}

fn builtin_type_name(b: BuiltinType) -> &'static str {
    match b {
        BuiltinType::U8 { lang_c_char: true } => {
            panic!("no type name for string or char8 builtins")
        }
        BuiltinType::U8 { lang_c_char: false } => "uint8_t",
        BuiltinType::U16 => "uint16_t",
        BuiltinType::U32 {
            lang_ptr_size: true,
        } => "size_t",
        BuiltinType::U32 {
            lang_ptr_size: false,
        } => "uint32_t",
        BuiltinType::U64 => "uint64_t",
        BuiltinType::S8 => "int8_t",
        BuiltinType::S16 => "int16_t",
        BuiltinType::S32 => "int32_t",
        BuiltinType::S64 => "int64_t",
        BuiltinType::F32 => "float",
        BuiltinType::F64 => "double",
        BuiltinType::Char => "char32_t",
    }
}

fn typeref_name(tref: &TypeRef) -> String {
    match &**tref.type_() {
        Type::Builtin(BuiltinType::U8 { lang_c_char: true }) | Type::List(_) => {
            panic!("unsupported grammar: cannot construct name of string or array",)
        }
        _ => {}
    }

    match tref {
        TypeRef::Name(named_type) => namedtype_name(&named_type),
        TypeRef::Value(anon_type) => match &**anon_type {
            Type::List(_) => unreachable!("lists excluded above"),
            Type::Builtin(b) => builtin_type_name(*b).to_string(),
            Type::Pointer(p) => format!("{}_ptr", typeref_name(&*p)),
            Type::ConstPointer(p) => format!("const_{}_ptr", typeref_name(&*p)),
            Type::Record { .. } | Type::Variant { .. } | Type::Handle { .. } => {
                unreachable!("wasi should not have anonymous record, variant, handles")
            }
        },
    }
}

fn namedtype_name(named_type: &NamedType) -> String {
    match &**named_type.type_() {
        Type::Pointer(p) => format!("{}_ptr", typeref_name(&*p)),
        Type::ConstPointer(p) => format!("const_{}_ptr", typeref_name(&*p)),
        Type::List(_) => unreachable!("arrays excluded above"),
        _ => format!("__wasi_{}_t", named_type.name.as_str()),
    }
}

fn intrepr_name(i: IntRepr) -> &'static str {
    match i {
        IntRepr::U8 => "uint8_t",
        IntRepr::U16 => "uint16_t",
        IntRepr::U32 => "uint32_t",
        IntRepr::U64 => "uint64_t",
    }
}
