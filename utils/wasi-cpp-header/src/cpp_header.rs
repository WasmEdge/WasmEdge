// SPDX-License-Identifier: Apache-2.0
use heck::ShoutySnakeCase;
use witx::*;

pub fn to_cpp_header(doc: &Document, inputs_str: &str) -> String {
    let mut ret = String::new();

    ret.push_str(&format!(
        r#"/**
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
            Type::Enum(e) => print_enum(ret, &nt.name, e),
            Type::Int(i) => print_int(ret, &nt.name, i),
            Type::Flags(f) => print_flags(ret, &nt.name, f),
            Type::Struct(s) => print_struct(ret, &nt.name, s),
            Type::Union(u) => print_union(ret, &nt.name, u),
            Type::Handle(h) => print_handle(ret, &nt.name, h),
            Type::Builtin { .. }
            | Type::Array { .. }
            | Type::Pointer { .. }
            | Type::ConstPointer { .. } => print_alias(ret, &nt.name, &nt.tref),
        },
        TypeRef::Name(_) => print_alias(ret, &nt.name, &nt.tref),
    }
}

fn print_alias(ret: &mut String, name: &Id, dest: &TypeRef) {
    match &*dest.type_() {
        Type::Array(_) => {
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

fn print_enum(ret: &mut String, name: &Id, e: &EnumDatatype) {
    ret.push_str(&format!(
        "enum __wasi_{}_t : {} {{\n",
        ident_name(name),
        intrepr_name(e.repr)
    ));

    for (index, variant) in e.variants.iter().enumerate() {
        if !variant.docs.is_empty() {
            ret.push_str("  /**\n");
            for line in variant.docs.lines() {
                ret.push_str(&format!("   * {}\n", line));
            }
            ret.push_str("   */\n");
        }
        ret.push_str(&format!(
            "  __WASI_{}_{} = {},\n",
            ident_name(&name).to_shouty_snake_case(),
            ident_name(&variant.name).to_shouty_snake_case(),
            index
        ));
        ret.push_str("\n");
    }
    ret.push_str("};\n");

    ret.push_str(&format!(
        "static_assert(sizeof(__wasi_{}_t) == {}, \"witx calculated size\");\n",
        ident_name(name),
        e.repr.mem_size()
    ));
    ret.push_str(&format!(
        "static_assert(alignof(__wasi_{}_t) == {}, \"witx calculated align\");\n",
        ident_name(name),
        e.repr.mem_align()
    ));

    ret.push_str("\n");
}

fn print_int(ret: &mut String, name: &Id, i: &IntDatatype) {
    ret.push_str(&format!(
        "enum __wasi_{}_t : {} {{\n",
        ident_name(name),
        intrepr_name(i.repr)
    ));

    for (index, const_) in i.consts.iter().enumerate() {
        if !const_.docs.is_empty() {
            ret.push_str("  /**\n");
            for line in const_.docs.lines() {
                ret.push_str(&format!("   * {}\n", line));
            }
            ret.push_str("   */\n");
        }
        ret.push_str(&format!(
            "  __WASI_{}_{} = {},\n",
            ident_name(&name).to_shouty_snake_case(),
            ident_name(&const_.name).to_shouty_snake_case(),
            index
        ));
        ret.push_str("\n");
    }
    ret.push_str("};\n");

    ret.push_str(&format!(
        "static_assert(sizeof(__wasi_{}_t) == {}, \"witx calculated size\");\n",
        ident_name(name),
        i.repr.mem_size()
    ));
    ret.push_str(&format!(
        "static_assert(alignof(__wasi_{}_t) == {}, \"witx calculated align\");\n",
        ident_name(name),
        i.repr.mem_align()
    ));

    ret.push_str("\n");
}

fn print_flags(ret: &mut String, name: &Id, f: &FlagsDatatype) {
    ret.push_str(&format!(
        "enum __wasi_{}_t : {} {{\n",
        ident_name(name),
        intrepr_name(f.repr)
    ));
    ret.push_str("\n");

    for (index, flag) in f.flags.iter().enumerate() {
        if !flag.docs.is_empty() {
            ret.push_str("  /**\n");
            for line in flag.docs.lines() {
                ret.push_str(&format!("   * {}\n", line));
            }
            ret.push_str("   */\n");
        }
        ret.push_str(&format!(
            "  __WASI_{}_{} = {},\n",
            ident_name(name).to_shouty_snake_case(),
            ident_name(&flag.name).to_shouty_snake_case(),
            1u128 << index
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
        f.repr.mem_size(),
    ));
    ret.push_str(&format!(
        "static_assert(alignof(__wasi_{}_t) == {}, \"witx calculated align\");\n",
        ident_name(name),
        f.repr.mem_align(),
    ));

    ret.push_str("\n");
}

fn print_struct(ret: &mut String, name: &Id, s: &StructDatatype) {
    ret.push_str(&format!(
        "using __wasi_{}_t = struct __wasi_{}_t {{\n",
        ident_name(name),
        ident_name(name)
    ));

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

fn print_union(ret: &mut String, name: &Id, u: &UnionDatatype) {
    ret.push_str(&format!(
        "using __wasi_{}_u_t = union __wasi_{}_u_t {{\n",
        ident_name(name),
        ident_name(name)
    ));

    for variant in &u.variants {
        if let Some(ref tref) = variant.tref {
            if !variant.docs.is_empty() {
                ret.push_str("  /**\n");
                for line in variant.docs.lines() {
                    ret.push_str(&format!("  * {}\n", line));
                }
                ret.push_str("  */\n");
            }
            ret.push_str(&format!(
                "  {} {};\n",
                typeref_name(tref),
                ident_name(&variant.name)
            ));
        }
    }
    ret.push_str(&format!("}};\n"));

    ret.push_str(&format!(
        "using __wasi_{}_t = struct __wasi_{}_t {{\n",
        ident_name(name),
        ident_name(name)
    ));

    ret.push_str(&format!("  {} tag;\n", namedtype_name(&u.tag)));
    ret.push_str(&format!("  __wasi_{}_u_t u;\n", ident_name(name)));

    ret.push_str(&format!("}};\n"));
    ret.push_str("\n");

    ret.push_str(&format!(
        "static_assert(sizeof(__wasi_{}_t) == {}, \"witx calculated size\");\n",
        ident_name(name),
        u.mem_size()
    ));
    ret.push_str(&format!(
        "static_assert(alignof(__wasi_{}_t) == {}, \"witx calculated align\");\n",
        ident_name(name),
        u.mem_align()
    ));

    let l = u.union_layout();
    ret.push_str(&format!(
        "static_assert(offsetof(__wasi_{}_t, u) == {}, \"witx calculated union offset\");\n",
        ident_name(name),
        l.contents_offset,
    ));
    ret.push_str(&format!(
        "static_assert(sizeof(__wasi_{}_u_t) == {}, \"witx calculated union size\");\n",
        ident_name(name),
        l.contents_size,
    ));
    ret.push_str(&format!(
        "static_assert(alignof(__wasi_{}_u_t) == {}, \"witx calculated union align\");\n",
        ident_name(name),
        l.contents_align,
    ));

    ret.push_str("\n");
}

fn print_handle(ret: &mut String, name: &Id, h: &HandleDatatype) {
    ret.push_str(&format!("using __wasi_{}_t = int32_t;\n\n", ident_name(name)));

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
        BuiltinType::String | BuiltinType::Char8 => {
            panic!("no type name for string or char8 builtins")
        }
        BuiltinType::USize => "size_t",
        BuiltinType::U8 => "uint8_t",
        BuiltinType::U16 => "uint16_t",
        BuiltinType::U32 => "uint32_t",
        BuiltinType::U64 => "uint64_t",
        BuiltinType::S8 => "int8_t",
        BuiltinType::S16 => "int16_t",
        BuiltinType::S32 => "int32_t",
        BuiltinType::S64 => "int64_t",
        BuiltinType::F32 => "float",
        BuiltinType::F64 => "double",
    }
}

fn typeref_name(tref: &TypeRef) -> String {
    match &*tref.type_() {
        Type::Builtin(BuiltinType::String) | Type::Builtin(BuiltinType::Char8) | Type::Array(_) => {
            panic!("unsupported grammar: cannot construct name of string or array",)
        }
        _ => {}
    }

    match tref {
        TypeRef::Name(named_type) => namedtype_name(&named_type),
        TypeRef::Value(anon_type) => match &**anon_type {
            Type::Array(_) => unreachable!("arrays excluded above"),
            Type::Builtin(b) => builtin_type_name(*b).to_string(),
            Type::Pointer(p) => format!("{}_ptr", typeref_name(&*p)),
            Type::ConstPointer(p) => format!("const_{}_ptr", typeref_name(&*p)),
            Type::Int(i) => format!("{}", intrepr_name(i.repr)),
            Type::Struct { .. }
            | Type::Union { .. }
            | Type::Enum { .. }
            | Type::Flags { .. }
            | Type::Handle { .. } => unreachable!(
                "wasi should not have anonymous structs, unions, enums, flags, handles"
            ),
        },
    }
}

fn namedtype_name(named_type: &NamedType) -> String {
    match &*named_type.type_() {
        Type::Pointer(p) => format!("{} *", typeref_name(&*p)),
        Type::ConstPointer(p) => format!("const {} *", typeref_name(&*p)),
        Type::Array(_) => unreachable!("arrays excluded above"),
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
