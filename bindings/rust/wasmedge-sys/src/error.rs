//! Defines the error types.

use crate::{
    ffi::{WasmEdge_Result, WasmEdge_ResultGetCode, WasmEdge_ResultOK},
    WasmEdgeResult,
};
use thiserror::Error;
use wasmedge_types::ExternalInstanceType;

/// Defines the errors raised by the wasmedge-sys crate.
#[derive(Error, Clone, Debug, PartialEq)]
pub enum WasmEdgeError {
    // c-api
    #[error("{0}")]
    Core(CoreError),

    // context
    #[error("Fail to create ImportObj module")]
    ImportObjCreate,
    #[error("Fail to create Executor context")]
    ExecutorCreate,
    #[error("{0}")]
    Store(StoreError),
    #[error("Fail to create Statistics context")]
    StatisticsCreate,
    #[error("Fail to create Module")]
    ModuleCreate,
    #[error("Fail to create Loader")]
    LoaderCreate,
    #[error("Fail to create Config")]
    ConfigCreate,
    #[error("Fail to create AOT Compiler")]
    CompilerCreate,
    #[error("Fail to create Validator")]
    ValidatorCreate,
    #[error("{0}")]
    Vm(VmError),

    // instances
    #[error("{0}")]
    Func(FuncError),
    #[error("Fail to create FuncType")]
    FuncTypeCreate,
    #[error("{0}")]
    Mem(MemError),
    #[error("Fail to create MemType")]
    MemTypeCreate,
    #[error("{0}")]
    Global(GlobalError),
    #[error("Fail to create GlobalType")]
    GlobalTypeCreate,
    #[error("{0}")]
    Table(TableError),
    #[error("Fail to create TableType")]
    TableTypeCreate,
    #[error("{0}")]
    Import(ImportError),
    #[error("{0}")]
    Export(ExportError),
    #[error("{0}")]
    Instance(InstanceError),

    // std
    #[error("Found an interior nul byte")]
    FoundNulByte(#[from] std::ffi::NulError),
    #[error("Fail to find a nul byte in the expected position")]
    NotFoundNulByte(#[from] std::ffi::FromBytesWithNulError),
    #[error("Fail to interpret a sequence of u8 as a string")]
    Utf8(#[from] std::str::Utf8Error),
}

/// Defines the errors raised from [Function](crate::Function).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum FuncError {
    #[error("Fail to create Function instance")]
    Create,
    #[error("{0}")]
    CreateBinding(String),
    #[error("Fail to get the function type")]
    Type,
}

/// Defines the errors raised from [Memory](crate::Memory).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum MemError {
    #[error("Fail to create Memory instance")]
    Create,
    #[error("Fail to get the memory type")]
    Type,
    #[error("Fail to get the const pointer to data")]
    ConstPtr,
    #[error("Fail to get the pointer to data")]
    MutPtr,
    #[error("Fail to convert a raw pointer to a reference")]
    Ptr2Ref,
}

/// Defines the errors raised from [Global](crate::Global).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum GlobalError {
    #[error("Fail to create Global instance")]
    Create,
    #[error("Fail to get the global type")]
    Type,
    #[error("Trying to set value to a const global variable")]
    ModifyConst,
    #[error("")]
    UnmatchedValType,
}

/// Defines the errors raised from [Table](crate::Table).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum TableError {
    #[error("Fail to create Table instance")]
    Create,
    #[error("Fail to get the table type")]
    Type,
}

/// Defines the errors raised from [ImportType](crate::ImportType).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum ImportError {
    #[error("The expected Import type is {expected:?}, but found {actual:?}")]
    Type {
        expected: ExternalInstanceType,
        actual: ExternalInstanceType,
    },
    #[error("{0}")]
    FuncType(String),
    #[error("{0}")]
    TableType(String),
    #[error("{0}")]
    MemType(String),
    #[error("{0}")]
    GlobalType(String),
}

/// Defines the errors raised from [ExportType](crate::ExportType).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum ExportError {
    #[error("The expected Export type is {expected:?}, but found {actual:?}")]
    Type {
        expected: ExternalInstanceType,
        actual: ExternalInstanceType,
    },
    #[error("{0}")]
    FuncType(String),
    #[error("{0}")]
    TableType(String),
    #[error("{0}")]
    MemType(String),
    #[error("{0}")]
    GlobalType(String),
}

/// Defines the errors raised from [Instance](crate::Instance).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum InstanceError {
    #[error("Fail to create Instance context")]
    Create,
    #[error("Fail to create WasiModule context")]
    CreateWasi,
    #[error("Fail to create WasmEdgeProcessModule context")]
    CreateWasmEdgeProcess,
    #[error("Fail to create ImportModule context")]
    CreateImportModule,
    #[error("Fail to find the target function ({0})")]
    NotFoundFunc(String),
    #[error("Fail to find the target table ({0})")]
    NotFoundTable(String),
    #[error("Fail to find the target memory ({0})")]
    NotFoundMem(String),
    #[error("Fail to find the target global ({0})")]
    NotFoundGlobal(String),
}

/// Defines the errors raised from [Store](crate::Store).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum StoreError {
    #[error("Fail to create Store context")]
    Create,
    #[error("Fail to find the target function ({0})")]
    NotFoundFunc(String),
    #[error("Fail to find the target function ({func_name}) in the module ({mod_name})")]
    NotFoundFuncRegistered { func_name: String, mod_name: String },
    #[error("Fail to find the target table ({0})")]
    NotFoundTable(String),
    #[error("Fail to find the target table ({table_name}) in the module ({mod_name})")]
    NotFoundTableRegistered {
        table_name: String,
        mod_name: String,
    },
    #[error("Fail to find the target memory ({0})")]
    NotFoundMem(String),
    #[error("Fail to find the target memory ({mem_name}) in the module ({mod_name})")]
    NotFoundMemRegistered { mem_name: String, mod_name: String },
    #[error("Fail to find the target global ({0})")]
    NotFoundGlobal(String),
    #[error("Fail to find the target global ({global_name}) in the module ({mod_name})")]
    NotFoundGlobalRegistered {
        global_name: String,
        mod_name: String,
    },
    #[error("Not found the target module ({0})")]
    NotFoundModule(String),
    #[error("Not found the active module")]
    NotFoundActiveModule,
}

/// Defines the errors raised from [Vm](crate::Vm).
#[derive(Error, Clone, Debug, PartialEq)]
pub enum VmError {
    #[error("Fail to create Vm context")]
    Create,
    #[error("Fail to get the type of the target function ({0})")]
    NotFoundFuncType(String),
    #[error("Fail to get Wasi ImportObject module")]
    NotFoundWasiImportObjectModule,
    #[error("Fail to get WasmEdgeProcess ImportObject module")]
    NotFoundWasmEdgeProcessImportObjectModule,
    #[error("Fail to get Store context")]
    NotFoundStore,
    #[error("Fail to get Statistics context")]
    NotFoundStatistics,
    #[error("Fail to get the target ImportModule (name: {0})")]
    NotFoundImportModule(String),
    #[error(
        "Fail to register import module. Another import module with the name has already existed."
    )]
    DuplicateImportModule,
    #[error("Fail to get Loader context")]
    NotFoundLoader,
    #[error("Fail to get Validator context")]
    NotFoundValidator,
    #[error("Fail to get Executor context")]
    NotFoundExecutor,
    #[error("Try to register an invalid import module")]
    InvalidImportModule,
    #[error("Not found active module instance")]
    NotFoundActiveModule,
}

/// Defines the errors raised from WasmEdge Core.
#[derive(Error, Clone, Debug, PartialEq)]
pub enum CoreError {
    #[error("{0}")]
    Common(CoreCommonError),
    #[error("{0}")]
    Load(CoreLoadError),
    #[error("{0}")]
    Validation(CoreValidationError),
    #[error("{0}")]
    Instantiation(CoreInstantiationError),
    #[error("{0}")]
    Execution(CoreExecutionError),
}

/// Defines the common errors.
#[derive(Error, Clone, Debug, PartialEq)]
pub enum CoreCommonError {
    #[error("generic runtime error")]
    RuntimeError,
    #[error("cost limit exceeded")]
    CostLimitExceeded,
    #[error("wrong VM workflow")]
    WrongVMWorkflow,
    #[error("wasm function not found")]
    FuncNotFound,
    #[error("AOT runtime is disabled in this build")]
    AOTDisabled,
    #[error("execution interrupted")]
    Interrupted,
}

/// Defines the errors raised in the load phase.
#[derive(Error, Clone, Debug, PartialEq)]
pub enum CoreLoadError {
    #[error("Invalid file path")]
    IllegalPath,
    #[error("Read error")]
    ReadError,
    #[error("unexpected end")]
    UnexpectedEnd,
    #[error("magic header not detected")]
    MalformedMagic,
    #[error("unknown binary version")]
    MalformedVersion,
    #[error("malformed section id")]
    MalformedSection,
    #[error("section size mismatch")]
    SectionSizeMismatch,
    #[error("length out of bounds")]
    NameSizeOutOfBounds,
    #[error("unexpected content after last section")]
    JunkSection,
    #[error("function and code section have inconsistent lengths")]
    IncompatibleFuncCode,
    #[error("data count and data section have inconsistent lengths")]
    IncompatibleDataCount,
    #[error("data count section required")]
    DataCountRequired,
    #[error("malformed import kind")]
    MalformedImportKind,
    #[error("malformed export kind")]
    MalformedExportKind,
    #[error("zero byte expected")]
    ExpectedZeroByte,
    #[error("malformed mutability")]
    InvalidMut,
    #[error("too many locals")]
    TooManyLocals,
    #[error("malformed value type")]
    MalformedValType,
    #[error("malformed element type")]
    MalformedElemType,
    #[error("malformed reference type")]
    MalformedRefType,
    #[error("malformed UTF-8 encoding")]
    MalformedUTF8,
    #[error("integer too large")]
    IntegerTooLarge,
    #[error("integer representation too long")]
    IntegerTooLong,
    #[error("illegal opcode")]
    IllegalOpCode,
    #[error("invalid wasm grammar")]
    IllegalGrammar,
}

/// Defines the errors raised in the validation phase.
#[derive(Error, Clone, Debug, PartialEq)]
pub enum CoreValidationError {
    #[error("alignment must not be larger than natural")]
    InvalidAlignment,
    #[error("type mismatch")]
    TypeCheckFailed,
    #[error("unknown label")]
    InvalidLabelIdx,
    #[error("unknown local")]
    InvalidLocalIdx,
    #[error("unknown type")]
    InvalidFuncTypeIdx,
    #[error("unknown function")]
    InvalidFuncIdx,
    #[error("unknown table")]
    InvalidTableIdx,
    #[error("unknown memory")]
    InvalidMemoryIdx,
    #[error("unknown global")]
    InvalidGlobalIdx,
    #[error("unknown elem segment")]
    InvalidElemIdx,
    #[error("unknown data segment")]
    InvalidDataIdx,
    #[error("undeclared function reference")]
    InvalidRefIdx,
    #[error("constant expression required")]
    ConstExprRequired,
    #[error("duplicate export name")]
    DupExportName,
    #[error("global is immutable")]
    ImmutableGlobal,
    #[error("invalid result arity")]
    InvalidResultArity,
    #[error("multiple tables")]
    MultiTables,
    #[error("multiple memories")]
    MultiMemories,
    #[error("size minimum must not be greater than maximum")]
    InvalidLimit,
    #[error("memory size must be at most 65536 pages (4GiB)")]
    InvalidMemPages,
    #[error("start function")]
    InvalidStartFunc,
    #[error("invalid lane index")]
    InvalidLaneIdx,
}

/// Defines the errors raised in the instantiation phase.
#[derive(Error, Clone, Debug, PartialEq)]
pub enum CoreInstantiationError {
    #[error("module name conflict")]
    ModuleNameConflict,
    #[error("incompatible import type")]
    IncompatibleImportType,
    #[error("unknown import")]
    UnknownImport,
    #[error("data segment does not fit")]
    DataSegDoesNotFit,
    #[error("elements segment does not fit")]
    ElemSegDoesNotFit,
}

/// Defines the errors raised in the execution phase.
#[derive(Error, Clone, Debug, PartialEq)]
pub enum CoreExecutionError {
    #[error("wrong instance address")]
    WrongInstanceAddress,
    #[error("wrong instance index")]
    WrongInstanceIndex,
    #[error("instruction type mismatch")]
    InstrTypeMismatch,
    #[error("function type mismatch")]
    FuncTypeMismatch,
    #[error("integer divide by zero")]
    DivideByZero,
    #[error("integer overflow")]
    IntegerOverflow,
    #[error("invalid conversion to integer")]
    InvalidConvToInt,
    #[error("out of bounds table access")]
    TableOutOfBounds,
    #[error("out of bounds memory access")]
    MemoryOutOfBounds,
    #[error("unreachable")]
    Unreachable,
    #[error("uninitialized element")]
    UninitializedElement,
    #[error("undefined element")]
    UndefinedElement,
    #[error("indirect call type mismatch")]
    IndirectCallTypeMismatch,
    #[error("host function failed")]
    ExecutionFailed,
    #[error("reference type mismatch")]
    RefTypeMismatch,
}

/// Converts WasmEdge_Result to WasmEdgeResult
pub(crate) fn check(result: WasmEdge_Result) -> WasmEdgeResult<()> {
    let code = unsafe {
        if !WasmEdge_ResultOK(result) {
            WasmEdge_ResultGetCode(result)
        } else {
            0u32
        }
    };
    match code {
        // Success or terminated (exit and return success)
        0x00 | 0x01 => Ok(()),

        // Common errors
        0x02 => Err(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::RuntimeError,
        ))),
        0x03 => Err(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::CostLimitExceeded,
        ))),
        0x04 => Err(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::WrongVMWorkflow,
        ))),
        0x05 => Err(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::FuncNotFound,
        ))),
        0x06 => Err(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::AOTDisabled,
        ))),
        0x07 => Err(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::Interrupted,
        ))),

        // Load phase
        0x20 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IllegalPath,
        ))),
        0x21 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::ReadError,
        ))),
        0x22 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::UnexpectedEnd,
        ))),
        0x23 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedMagic,
        ))),
        0x24 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedVersion,
        ))),
        0x25 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedSection,
        ))),
        0x26 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::SectionSizeMismatch,
        ))),
        0x27 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::NameSizeOutOfBounds,
        ))),
        0x28 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::JunkSection,
        ))),
        0x29 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IncompatibleFuncCode,
        ))),
        0x2A => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IncompatibleDataCount,
        ))),
        0x2B => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::DataCountRequired,
        ))),
        0x2C => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedImportKind,
        ))),
        0x2D => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedExportKind,
        ))),
        0x2E => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::ExpectedZeroByte,
        ))),
        0x2F => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::InvalidMut,
        ))),
        0x30 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::TooManyLocals,
        ))),
        0x31 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedValType,
        ))),
        0x32 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedElemType,
        ))),
        0x33 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedRefType,
        ))),
        0x34 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedUTF8,
        ))),
        0x35 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IntegerTooLarge,
        ))),
        0x36 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IntegerTooLong,
        ))),
        0x37 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IllegalOpCode,
        ))),
        0x38 => Err(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IllegalGrammar,
        ))),

        // Validation phase
        0x40 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidAlignment,
        ))),
        0x41 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::TypeCheckFailed,
        ))),
        0x42 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidLabelIdx,
        ))),
        0x43 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidLocalIdx,
        ))),
        0x44 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidFuncTypeIdx,
        ))),
        0x45 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidFuncIdx,
        ))),
        0x46 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidTableIdx,
        ))),
        0x47 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidMemoryIdx,
        ))),
        0x48 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidGlobalIdx,
        ))),
        0x49 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidElemIdx,
        ))),
        0x4A => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidDataIdx,
        ))),
        0x4B => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidRefIdx,
        ))),
        0x4C => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::ConstExprRequired,
        ))),
        0x4D => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::DupExportName,
        ))),
        0x4E => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::ImmutableGlobal,
        ))),
        0x4F => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidResultArity,
        ))),
        0x50 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::MultiTables,
        ))),
        0x51 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::MultiMemories,
        ))),
        0x52 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidLimit,
        ))),
        0x53 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidMemPages,
        ))),
        0x54 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidStartFunc,
        ))),
        0x55 => Err(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidLaneIdx,
        ))),

        // Instantiation phase
        0x60 => Err(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::ModuleNameConflict,
        ))),
        0x61 => Err(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::IncompatibleImportType,
        ))),
        0x62 => Err(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::UnknownImport,
        ))),
        0x63 => Err(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::DataSegDoesNotFit,
        ))),
        0x64 => Err(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::ElemSegDoesNotFit,
        ))),

        // Execution phase
        0x80 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::WrongInstanceAddress,
        ))),
        0x81 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::WrongInstanceIndex,
        ))),
        0x82 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::InstrTypeMismatch,
        ))),
        0x83 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::FuncTypeMismatch,
        ))),
        0x84 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::DivideByZero,
        ))),
        0x85 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::IntegerOverflow,
        ))),
        0x86 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::InvalidConvToInt,
        ))),
        0x87 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::TableOutOfBounds,
        ))),
        0x88 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::MemoryOutOfBounds,
        ))),
        0x89 => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::Unreachable,
        ))),
        0x8A => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::UninitializedElement,
        ))),
        0x8B => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::UndefinedElement,
        ))),
        0x8C => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::IndirectCallTypeMismatch,
        ))),
        0x8D => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::ExecutionFailed,
        ))),
        0x8E => Err(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::RefTypeMismatch,
        ))),

        _ => panic!("unknown error code: {}", code),
    }
}
