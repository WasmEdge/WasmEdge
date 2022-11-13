//! The [wasmedge-types](https://crates.io/crates/wasmedge-types) crate defines a group of common data structures used by both [wasmedge-rs](https://crates.io/crates/wasmedge-sdk) and [wasmedge-sys](https://crates.io/crates/wasmedge-sys) crates.
//!
//! See also
//!
//! * [WasmEdge Runtime](https://wasmedge.org/)

pub mod error;

/// Defines WasmEdge reference types.
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum RefType {
    /// Refers to the infinite union of all references to host functions, regardless of their function types.
    FuncRef,

    /// Refers to the infinite union of all references to objects and that can be passed into WebAssembly under this type.
    ExternRef,
}
impl From<u32> for RefType {
    fn from(value: u32) -> Self {
        match value {
            112 => RefType::FuncRef,
            111 => RefType::ExternRef,
            _ => panic!("[wasmedge-types] Invalid WasmEdge_RefType: {value:#X}"),
        }
    }
}
impl From<RefType> for u32 {
    fn from(value: RefType) -> Self {
        match value {
            RefType::FuncRef => 112,
            RefType::ExternRef => 111,
        }
    }
}
impl From<i32> for RefType {
    fn from(value: i32) -> Self {
        match value {
            112 => RefType::FuncRef,
            111 => RefType::ExternRef,
            _ => panic!("[wasmedge-types] Invalid WasmEdge_RefType: {value:#X}"),
        }
    }
}
impl From<RefType> for i32 {
    fn from(value: RefType) -> Self {
        match value {
            RefType::FuncRef => 112,
            RefType::ExternRef => 111,
        }
    }
}

/// Defines WasmEdge value types.
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
    /// A reference to a host function.
    FuncRef,
    /// A reference to object.
    ExternRef,
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
            _ => panic!("[wasmedge-types] Invalid WasmEdge_ValType: {value:#X}"),
        }
    }
}
impl From<ValType> for u32 {
    fn from(value: ValType) -> Self {
        match value {
            ValType::I32 => 127,
            ValType::I64 => 126,
            ValType::F32 => 125,
            ValType::F64 => 124,
            ValType::V128 => 123,
            ValType::FuncRef => 112,
            ValType::ExternRef => 111,
        }
    }
}
impl From<i32> for ValType {
    fn from(value: i32) -> Self {
        match value {
            127 => ValType::I32,
            126 => ValType::I64,
            125 => ValType::F32,
            124 => ValType::F64,
            123 => ValType::V128,
            112 => ValType::FuncRef,
            111 => ValType::ExternRef,
            _ => panic!("[wasmedge-types] Invalid WasmEdge_ValType: {value:#X}"),
        }
    }
}
impl From<ValType> for i32 {
    fn from(value: ValType) -> Self {
        match value {
            ValType::I32 => 127,
            ValType::I64 => 126,
            ValType::F32 => 125,
            ValType::F64 => 124,
            ValType::V128 => 123,
            ValType::FuncRef => 112,
            ValType::ExternRef => 111,
        }
    }
}

/// Defines the mutability property of WasmEdge Global variables.
///
/// `Mutability` determines the mutability property of a WasmEdge Global variable is either mutable or immutable.
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum Mutability {
    /// Identifies an immutable global variable.
    Const,
    /// Identifies a mutable global variable.
    Var,
}
impl From<u32> for Mutability {
    fn from(value: u32) -> Self {
        match value {
            0 => Mutability::Const,
            1 => Mutability::Var,
            _ => panic!("[wasmedge-types] Invalid WasmEdge_Mutability: {value:#X}"),
        }
    }
}
impl From<Mutability> for u32 {
    fn from(value: Mutability) -> Self {
        match value {
            Mutability::Const => 0,
            Mutability::Var => 1,
        }
    }
}
impl From<i32> for Mutability {
    fn from(value: i32) -> Self {
        match value {
            0 => Mutability::Const,
            1 => Mutability::Var,
            _ => panic!("[wasmedge-types] Invalid WasmEdge_Mutability: {value:#X}"),
        }
    }
}
impl From<Mutability> for i32 {
    fn from(value: Mutability) -> Self {
        match value {
            Mutability::Const => 0,
            Mutability::Var => 1,
        }
    }
}

/// Defines WasmEdge AOT compiler optimization level.
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
impl From<u32> for CompilerOptimizationLevel {
    fn from(val: u32) -> CompilerOptimizationLevel {
        match val {
            0 => CompilerOptimizationLevel::O0,
            1 => CompilerOptimizationLevel::O1,
            2 => CompilerOptimizationLevel::O2,
            3 => CompilerOptimizationLevel::O3,
            4 => CompilerOptimizationLevel::Os,
            5 => CompilerOptimizationLevel::Oz,
            _ => panic!("Unknown CompilerOptimizationLevel value: {val}"),
        }
    }
}
impl From<CompilerOptimizationLevel> for u32 {
    fn from(val: CompilerOptimizationLevel) -> u32 {
        match val {
            CompilerOptimizationLevel::O0 => 0,
            CompilerOptimizationLevel::O1 => 1,
            CompilerOptimizationLevel::O2 => 2,
            CompilerOptimizationLevel::O3 => 3,
            CompilerOptimizationLevel::Os => 4,
            CompilerOptimizationLevel::Oz => 5,
        }
    }
}
impl From<i32> for CompilerOptimizationLevel {
    fn from(val: i32) -> CompilerOptimizationLevel {
        match val {
            0 => CompilerOptimizationLevel::O0,
            1 => CompilerOptimizationLevel::O1,
            2 => CompilerOptimizationLevel::O2,
            3 => CompilerOptimizationLevel::O3,
            4 => CompilerOptimizationLevel::Os,
            5 => CompilerOptimizationLevel::Oz,
            _ => panic!("Unknown CompilerOptimizationLevel value: {val}"),
        }
    }
}
impl From<CompilerOptimizationLevel> for i32 {
    fn from(val: CompilerOptimizationLevel) -> i32 {
        match val {
            CompilerOptimizationLevel::O0 => 0,
            CompilerOptimizationLevel::O1 => 1,
            CompilerOptimizationLevel::O2 => 2,
            CompilerOptimizationLevel::O3 => 3,
            CompilerOptimizationLevel::Os => 4,
            CompilerOptimizationLevel::Oz => 5,
        }
    }
}

/// Defines WasmEdge AOT compiler output binary format.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CompilerOutputFormat {
    /// Native dynamic library format.
    Native,

    /// WebAssembly with AOT compiled codes in custom sections.
    Wasm,
}
impl From<u32> for CompilerOutputFormat {
    fn from(val: u32) -> CompilerOutputFormat {
        match val {
            0 => CompilerOutputFormat::Native,
            1 => CompilerOutputFormat::Wasm,
            _ => panic!("Unknown CompilerOutputFormat value: {val}"),
        }
    }
}
impl From<CompilerOutputFormat> for u32 {
    fn from(val: CompilerOutputFormat) -> u32 {
        match val {
            CompilerOutputFormat::Native => 0,
            CompilerOutputFormat::Wasm => 1,
        }
    }
}
impl From<i32> for CompilerOutputFormat {
    fn from(val: i32) -> CompilerOutputFormat {
        match val {
            0 => CompilerOutputFormat::Native,
            1 => CompilerOutputFormat::Wasm,
            _ => panic!("Unknown CompilerOutputFormat value: {val}"),
        }
    }
}
impl From<CompilerOutputFormat> for i32 {
    fn from(val: CompilerOutputFormat) -> i32 {
        match val {
            CompilerOutputFormat::Native => 0,
            CompilerOutputFormat::Wasm => 1,
        }
    }
}

/// Defines WasmEdge host module registration enum.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum HostRegistration {
    Wasi,
    WasmEdgeProcess,
}
impl From<u32> for HostRegistration {
    fn from(val: u32) -> Self {
        match val {
            0 => HostRegistration::Wasi,
            1 => HostRegistration::WasmEdgeProcess,
            _ => panic!("Unknown WasmEdge_HostRegistration value: {val}"),
        }
    }
}
impl From<HostRegistration> for u32 {
    fn from(val: HostRegistration) -> u32 {
        match val {
            HostRegistration::Wasi => 0,
            HostRegistration::WasmEdgeProcess => 1,
        }
    }
}

/// Defines the type of external WasmEdge instances.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ExternalInstanceType {
    /// A WasmEdge instance that is a WasmEdge Func.
    Func(FuncType),
    /// A WasmEdge instance that is a WasmEdge Table.
    Table(TableType),
    /// A WasmEdge instance that is a WasmEdge Memory.
    Memory(MemoryType),
    /// A WasmEdge instance that is a WasmEdge Global.
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
impl From<i32> for ExternalInstanceType {
    fn from(value: i32) -> Self {
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
        write!(f, "{message}")
    }
}

/// Struct of WasmEdge FuncType.
///
/// A [FuncType] is used to declare the types of the parameters and return values of a WasmEdge Func to be created.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct FuncType {
    args: Option<Vec<ValType>>,
    returns: Option<Vec<ValType>>,
}
impl FuncType {
    /// Creates a new [FuncType] with the given types of arguments and returns.
    ///
    /// # Arguments
    ///
    /// * `args` - A vector of [ValType]s that represent the types of the arguments.
    ///
    /// * `returns` - A vector of [ValType]s that represent the types of the returns.
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

    /// Returns the number of the arguments of a host function.
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

    /// Returns the number of the returns of a host function.
    pub fn returns_len(&self) -> u32 {
        match &self.returns {
            Some(returns) => returns.len() as u32,
            None => 0,
        }
    }
}

/// Struct of WasmEdge TableType.
///
/// A [TableType] is used to declare the element type and the size range of a WasmEdge Table to be created.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TableType {
    elem_ty: RefType,
    min: u32,
    max: Option<u32>,
}
impl TableType {
    /// Creates a new [TableType] with the given element type and the size range.
    ///
    /// # Arguments
    ///
    /// * `elem_ty` - The element type of the table to be created.
    ///
    /// * `min` - The minimum size of the table to be created.
    ///
    /// * `max` - The maximum size of the table to be created.    
    pub fn new(elem_ty: RefType, min: u32, max: Option<u32>) -> Self {
        Self { elem_ty, min, max }
    }

    /// Returns the element type defined in the [TableType].
    pub fn elem_ty(&self) -> RefType {
        self.elem_ty
    }

    /// Returns the minimum size defined in the [TableType].
    pub fn minimum(&self) -> u32 {
        self.min
    }

    /// Returns the maximum size defined in the [TableType].
    pub fn maximum(&self) -> Option<u32> {
        self.max
    }
}
impl Default for TableType {
    fn default() -> Self {
        Self {
            elem_ty: RefType::FuncRef,
            min: 0,
            max: None,
        }
    }
}

/// Struct of WasmEdge MemoryType.
///
/// A [MemoryType] is used to declare the size range of a WasmEdge Memory to be created.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct MemoryType {
    min: u32,
    max: Option<u32>,
    shared: bool,
}
impl MemoryType {
    /// Creates a new [MemoryType] with the given size range.
    ///
    /// # Arguments
    ///
    /// * `min` - The minimum size of the memory to be created.
    ///
    /// * `max` - The maximum size of the memory to be created. If `shared` is set to true, `max` must be set.
    ///
    /// * `shared` - Enables shared memory if true.
    pub fn new(min: u32, max: Option<u32>, shared: bool) -> WasmEdgeResult<Self> {
        if shared && max.is_none() {
            return Err(Box::new(error::WasmEdgeError::Mem(
                error::MemError::CreateSharedType,
            )));
        }
        Ok(Self { min, max, shared })
    }

    /// Returns the minimum size defined in the [MemoryType].
    pub fn minimum(&self) -> u32 {
        self.min
    }

    /// Returns the maximum size defined in the [MemoryType].
    pub fn maximum(&self) -> Option<u32> {
        self.max
    }

    /// Returns whether the memory is shared.
    pub fn shared(&self) -> bool {
        self.shared
    }
}

/// Struct of WasmEdge GlobalType.
///
/// A [GlobalType] is used to declare the type of a WasmEdge Global to be created.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct GlobalType {
    ty: ValType,
    mutability: Mutability,
}
impl GlobalType {
    /// Creates a new [GlobalType] with the given value type and mutability.
    ///
    /// # Arguments
    ///
    /// * `ty` - The value type of the global to be created.
    ///
    /// * `mutability` - The value mutability property of the global to be created.
    pub fn new(ty: ValType, mutability: Mutability) -> Self {
        Self { ty, mutability }
    }

    /// Returns the value type defined in the [GlobalType].
    pub fn value_ty(&self) -> ValType {
        self.ty
    }

    /// Returns the value mutability property defined in the [GlobalType].
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

/// Parses in-memory bytes as either the [WebAssembly Text format](http://webassembly.github.io/spec/core/text/index.html), or a binary WebAssembly module.
pub use wat::parse_bytes as wat2wasm;

/// The WasmEdge result type.
pub type WasmEdgeResult<T> = Result<T, Box<error::WasmEdgeError>>;
