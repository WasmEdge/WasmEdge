//! Defines WasmEdge error types.

use crate::ExternalInstanceType;
use thiserror::Error;

/// The error types used by both wasmedge-sys and wasmedge crates.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
pub enum WasmEdgeError {
    // For general operation error
    #[error("{0}")]
    Operation(String),

    // For user-defined error
    #[error("{0}")]
    User(u32),

    /// Errors raised by WasmEdge Core.
    #[error("{0}")]
    Core(CoreError),

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
    #[error("Fail to convert a vector of bytes to a string")]
    FromUtf8(#[from] std::string::FromUtf8Error),

    // Windows platform
    #[error("Fail to convert path on Windows: {0}")]
    WindowsPathConversion(String),
}

/// The error types for WasmEdge Function.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
pub enum FuncError {
    #[error("Fail to create Function instance")]
    Create,
    #[error("{0}")]
    CreateBinding(String),
    #[error("Fail to get the function type")]
    Type,
}

/// The error types for WasmEdge Memory.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
pub enum MemError {
    #[error("Fail to create Memory instance")]
    Create,
    #[error("Fail to create shared memory type. The 'max' field must not be None.")]
    CreateSharedType,
    #[error("Fail to get the memory type")]
    Type,
    #[error("Fail to get the const pointer to data")]
    ConstPtr,
    #[error("Fail to get the pointer to data")]
    MutPtr,
    #[error("Fail to convert a raw pointer to a reference")]
    Ptr2Ref,
}

/// The error types for WasmEdge Global.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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

/// The error types for WasmEdge Table.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
pub enum TableError {
    #[error("Fail to create Table instance")]
    Create,
    #[error("Fail to get the table type")]
    Type,
}

/// The error types for WasmEdge ImportType.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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

/// The error types for WasmEdge ExportType.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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

/// The error types for WasmEdge Instance.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
pub enum InstanceError {
    #[error("Fail to create Instance context")]
    Create,
    #[error("Fail to create WasiModule context")]
    CreateWasi,
    #[error("Fail to create WasmEdgeProcessModule context")]
    CreateWasmEdgeProcess,
    #[error("Fail to create ImportModule context")]
    CreateImportModule,
    #[error("Not found the target function ({0})")]
    NotFoundFunc(String),
    #[error("Not found the target table ({0})")]
    NotFoundTable(String),
    #[error("Not found the target memory ({0})")]
    NotFoundMem(String),
    #[error("Not found the target global ({0})")]
    NotFoundGlobal(String),
    #[error("Not found the given mapped Fd/handler")]
    NotFoundMappedFdHandler,
}

/// The error types for WasmEdge Store.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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

/// The error types for WasmEdge Vm.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
pub enum VmError {
    #[error("Fail to create Vm context")]
    Create,
    #[error("Fail to get the type of the target function ({0})")]
    NotFoundFuncType(String),
    #[error("Fail to get module instance ({0})")]
    NotFoundModule(String),
    #[error("Fail to get Wasi module instance")]
    NotFoundWasiModule,
    #[error("Fail to get WasmEdge_Process module instance")]
    NotFoundWasmEdgeProcessModule,
    #[error("Fail to get WasiNn module instance")]
    NotFoundWasiNnModule,
    #[error("Fail to get WasiCryptoCommon module instance")]
    NotFoundWasiCryptoCommonModule,
    #[error("Fail to get WasiCryptoAsymmetricCommon module instance")]
    NotFoundWasiCryptoAsymmetricCommonModule,
    #[error("Fail to get WasiCryptoSymmetric module instance")]
    NotFoundWasiCryptoSymmetricModule,
    #[error("Fail to get WasiCryptoKx module instance")]
    NotFoundWasiCryptoKxModule,
    #[error("Fail to get WasiCryptoSignatures module instance")]
    NotFoundWasiCryptoSignaturesModule,
    #[error("Fail to get Store context")]
    NotFoundStore,
    #[error("Fail to get Statistics context")]
    NotFoundStatistics,
    #[error("Fail to get the target ImportModule (name: {0})")]
    NotFoundImportModule(String),
    #[error(
        "Fail to register import module named {0}. Another import module with the same name has already existed."
    )]
    DuplicateImportModule(String),
    #[error(
        "Fail to register module named {0}. Another module instance with the same name has already existed."
    )]
    DuplicateModuleInstance(String),
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

/// The error types raised by WasmEdge Core.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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

/// The error type for the common errors from WasmEdge Core.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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
    #[error("user defined error code")]
    UserDefError,
    #[error("wasm module hasn't passed validation yet")]
    NotValidated,
}

/// The error type for the load phase from WasmEdge Core.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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

/// The error type for the validation phase from WasmEdge Core.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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

/// The error type for the instantiation phase from WasmEdge Core.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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

/// The error type for the execution phase from WasmEdge Core.
#[derive(Error, Clone, Debug, PartialEq, Eq)]
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
    HostFuncFailed,
    #[error("reference type mismatch")]
    RefTypeMismatch,
    #[error("unaligned atomic")]
    UnalignedAtomicAccess,
    #[error("expected shared memory")]
    ExpectSharedMemory,
}

#[derive(Error, Clone, Debug, PartialEq, Eq)]
pub enum HostFuncError {
    #[error("User error: {0}")]
    User(u32),
    #[error("Runtime error: {0}")]
    Runtime(u32),
}
