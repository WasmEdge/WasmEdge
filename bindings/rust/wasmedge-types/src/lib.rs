#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum RefType {
    /// `FuncRef` denotes the infinite union of all references to [functions](crate::Function), regardless of their
    /// [function types](crate::FuncType).
    FuncRef,

    /// `ExternRef` denotes the infinite union of all references to objects owned by the [Vm](crate::Vm) and that can be
    /// passed into WebAssembly under this type.
    ExternRef,
}
impl From<u32> for RefType {
    fn from(value: u32) -> Self {
        match value {
            112 => RefType::FuncRef,
            111 => RefType::ExternRef,
            _ => panic!("[wasmedge-types] Invalid WasmEdge_RefType: {:#X}", value),
        }
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum ValType {
    /// 32-bit integer.
    ///
    /// Integers are not inherently signed or unsigned, their interpretation is determined by individual operations.
    I32,
    /// 64-bit integer.
    ///
    /// Integers are not inherently signed or unsigned, their interpretation is determined by individual operations.
    I64,
    /// 32-bit floating-point data as defined by the [IEEE 754-2019](https://ieeexplore.ieee.org/document/8766229).
    F32,
    /// 64-bit floating-point data as defined by the [IEEE 754-2019](https://ieeexplore.ieee.org/document/8766229).
    F64,
    /// 128-bit vector of packed integer or floating-point data.
    ///
    /// The packed data can be interpreted as signed or unsigned integers, single or double precision floating-point
    /// values, or a single 128 bit type. The interpretation is determined by individual operations.
    V128,
    /// A reference to [functions](crate::Function).
    FuncRef,
    /// A reference to object owned by the [Vm](crate::Vm).
    ExternRef,
    /// Unknown.
    None,
}
impl From<u32> for ValType {
    fn from(value: u32) -> Self {
        match value {
            127 => ValType::I32,
            126 => ValType::I64,
            125 => ValType::F32,
            124 => ValType::F64,
            123 => ValType::V128,
            112 => ValType::FuncRef,
            111 => ValType::ExternRef,
            64 => ValType::None,
            _ => panic!("[wasmedge-types] Invalid WasmEdge_ValType: {:#X}", value),
        }
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum Mutability {
    /// Identifies an immutable global variable
    Const,
    /// Identifies a mutable global variable
    Var,
}
impl From<u32> for Mutability {
    fn from(value: u32) -> Self {
        match value {
            0 => Mutability::Const,
            1 => Mutability::Var,
            _ => panic!("[wasmedge-types] Invalid WasmEdge_Mutability: {:#X}", value),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CompilerOptimizationLevel {
    /// Disable as many optimizations as possible.
    O0,

    /// Optimize quickly without destroying debuggability.
    O1,

    /// Optimize for fast execution as much as possible without triggering significant incremental compile time or code size growth.
    O2,

    ///  Optimize for fast execution as much as possible.
    O3,

    ///  Optimize for small code size as much as possible without triggering
    ///  significant incremental compile time or execution time slowdowns.
    Os,

    /// Optimize for small code size as much as possible.
    Oz,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CompilerOutputFormat {
    /// Native dynamic library format.
    Native,

    /// WebAssembly with AOT compiled codes in custom sections.
    Wasm,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum HostRegistration {
    Wasi,
    WasmEdgeProcess,
}

/// Defines WasmEdge ExternalType values.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ExternalInstanceType {
    Func(FuncType),
    Table(TableType),
    Memory(MemoryType),
    Global(GlobalType),
}
impl From<u32> for ExternalInstanceType {
    fn from(value: u32) -> Self {
        match value {
            0 => ExternalInstanceType::Func(FuncType::default()),
            1 => ExternalInstanceType::Table(TableType::default()),
            2 => ExternalInstanceType::Memory(MemoryType::default()),
            3 => ExternalInstanceType::Global(GlobalType::default()),
            _ => panic!(
                "[wasmedge-types] Invalid WasmEdge_ExternalType: {:#X}",
                value
            ),
        }
    }
}
impl std::fmt::Display for ExternalInstanceType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let message = match self {
            ExternalInstanceType::Func(_) => "function",
            ExternalInstanceType::Table(_) => "table",
            ExternalInstanceType::Memory(_) => "memory",
            ExternalInstanceType::Global(_) => "global",
        };
        write!(f, "{}", message)
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct FuncType {
    args: Option<Vec<ValType>>,
    returns: Option<Vec<ValType>>,
}
impl FuncType {
    pub fn new(args: Option<Vec<ValType>>, returns: Option<Vec<ValType>>) -> Self {
        Self { args, returns }
    }

    /// Returns the types of the arguments of a host function.
    pub fn args(&self) -> Option<&[ValType]> {
        match &self.args {
            Some(args) => Some(args.as_ref()),
            None => None,
        }
    }

    pub fn args_len(&self) -> u32 {
        match &self.args {
            Some(args) => args.len() as u32,
            None => 0,
        }
    }

    /// Returns the types of the returns of a host function.
    pub fn returns(&self) -> Option<&[ValType]> {
        match &self.returns {
            Some(returns) => Some(returns.as_ref()),
            None => None,
        }
    }

    pub fn returns_len(&self) -> u32 {
        match &self.returns {
            Some(returns) => returns.len() as u32,
            None => 0,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TableType {
    elem_ty: RefType,
    min: u32,
    max: u32,
}
impl TableType {
    pub fn new(elem_ty: RefType, min: u32, max: Option<u32>) -> Self {
        let max = match max {
            Some(val) => val,
            None => u32::MAX,
        };
        Self { elem_ty, min, max }
    }

    pub fn elem_ty(&self) -> RefType {
        self.elem_ty
    }

    pub fn minimum(&self) -> u32 {
        self.min
    }

    pub fn maximum(&self) -> u32 {
        self.max
    }
}
impl Default for TableType {
    fn default() -> Self {
        Self {
            elem_ty: RefType::FuncRef,
            min: 0,
            max: u32::MAX,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MemoryType {
    min: u32,
    max: u32,
}
impl MemoryType {
    pub fn new(min: u32, max: Option<u32>) -> Self {
        let max = match max {
            Some(max) => max,
            None => u32::MAX,
        };
        Self { min, max }
    }

    pub fn minimum(&self) -> u32 {
        self.min
    }

    pub fn maximum(&self) -> u32 {
        self.max
    }
}
impl Default for MemoryType {
    fn default() -> Self {
        Self {
            min: 0,
            max: u32::MAX,
        }
    }
}

/// Struct of WasmEdge GlobalType.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct GlobalType {
    ty: ValType,
    mutability: Mutability,
}
impl GlobalType {
    pub fn new(ty: ValType, mutability: Mutability) -> Self {
        Self { ty, mutability }
    }

    pub fn value_ty(&self) -> ValType {
        self.ty
    }

    pub fn mutability(&self) -> Mutability {
        self.mutability
    }
}
impl Default for GlobalType {
    fn default() -> Self {
        Self {
            ty: ValType::I32,
            mutability: Mutability::Var,
        }
    }
}
