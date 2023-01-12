//! Defines the versioning and logging functions.

use crate::{
    error::{
        CoreCommonError, CoreError, CoreExecutionError, CoreInstantiationError, CoreLoadError,
        CoreValidationError, WasmEdgeError,
    },
    ffi::{self, WasmEdge_Result, WasmEdge_ResultGetCode, WasmEdge_ResultOK},
    WasmEdgeResult,
};
use std::{
    ffi::{CStr, CString},
    path::Path,
};

#[cfg(unix)]
pub(crate) fn path_to_cstring(path: &Path) -> WasmEdgeResult<CString> {
    use std::os::unix::ffi::OsStrExt;
    CString::new(path.as_os_str().as_bytes())
        .map_err(|err| Box::new(WasmEdgeError::FoundNulByte(err)))
}

#[cfg(windows)]
pub(crate) fn path_to_cstring(path: &Path) -> WasmEdgeResult<CString> {
    match path.to_str() {
        Some(s) => CString::new(s).map_err(|err| Box::new(WasmEdgeError::FoundNulByte(err))),
        None => Err(Box::new(WasmEdgeError::WindowsPathConversion(
            path.to_string_lossy().to_string(),
        ))),
    }
}

/// Logs the debug information.
pub fn log_debug_info() {
    unsafe { ffi::WasmEdge_LogSetDebugLevel() }
}

/// Logs the error information.
pub fn log_error_info() {
    unsafe { ffi::WasmEdge_LogSetErrorLevel() }
}

/// Sets the logging system off.
pub fn log_off() {
    unsafe { ffi::WasmEdge_LogOff() }
}

// Checks the result of a `FFI` function.
pub(crate) fn check(result: WasmEdge_Result) -> WasmEdgeResult<()> {
    let category = unsafe { ffi::WasmEdge_ResultGetCategory(result) };
    let code = unsafe {
        if !WasmEdge_ResultOK(result) {
            WasmEdge_ResultGetCode(result)
        } else {
            0u32
        }
    };

    match category {
        ffi::WasmEdge_ErrCategory_UserLevelError => Err(Box::new(WasmEdgeError::User(code))),
        ffi::WasmEdge_ErrCategory_WASM => gen_runtime_error(code),
        _ => panic!("Invalid category value: {category}"),
    }
}

fn gen_runtime_error(code: u32) -> WasmEdgeResult<()> {
    match code {
        // Success or terminated (exit and return success)
        0x00 | 0x01 => Ok(()),

        // Common errors
        0x02 => Err(Box::new(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::RuntimeError,
        )))),
        0x03 => Err(Box::new(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::CostLimitExceeded,
        )))),
        0x04 => Err(Box::new(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::WrongVMWorkflow,
        )))),
        0x05 => Err(Box::new(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::FuncNotFound,
        )))),
        0x06 => Err(Box::new(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::AOTDisabled,
        )))),
        0x07 => Err(Box::new(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::Interrupted,
        )))),
        0x08 => Err(Box::new(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::NotValidated,
        )))),
        0x09 => Err(Box::new(WasmEdgeError::Core(CoreError::Common(
            CoreCommonError::UserDefError,
        )))),

        // Load phase
        0x20 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IllegalPath,
        )))),
        0x21 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::ReadError,
        )))),
        0x22 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::UnexpectedEnd,
        )))),
        0x23 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedMagic,
        )))),
        0x24 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedVersion,
        )))),
        0x25 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedSection,
        )))),
        0x26 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::SectionSizeMismatch,
        )))),
        0x27 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::NameSizeOutOfBounds,
        )))),
        0x28 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::JunkSection,
        )))),
        0x29 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IncompatibleFuncCode,
        )))),
        0x2A => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IncompatibleDataCount,
        )))),
        0x2B => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::DataCountRequired,
        )))),
        0x2C => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedImportKind,
        )))),
        0x2D => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedExportKind,
        )))),
        0x2E => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::ExpectedZeroByte,
        )))),
        0x2F => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::InvalidMut,
        )))),
        0x30 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::TooManyLocals,
        )))),
        0x31 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedValType,
        )))),
        0x32 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedElemType,
        )))),
        0x33 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedRefType,
        )))),
        0x34 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::MalformedUTF8,
        )))),
        0x35 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IntegerTooLarge,
        )))),
        0x36 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IntegerTooLong,
        )))),
        0x37 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IllegalOpCode,
        )))),
        0x38 => Err(Box::new(WasmEdgeError::Core(CoreError::Load(
            CoreLoadError::IllegalGrammar,
        )))),

        // Validation phase
        0x40 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidAlignment,
        )))),
        0x41 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::TypeCheckFailed,
        )))),
        0x42 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidLabelIdx,
        )))),
        0x43 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidLocalIdx,
        )))),
        0x44 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidFuncTypeIdx,
        )))),
        0x45 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidFuncIdx,
        )))),
        0x46 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidTableIdx,
        )))),
        0x47 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidMemoryIdx,
        )))),
        0x48 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidGlobalIdx,
        )))),
        0x49 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidElemIdx,
        )))),
        0x4A => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidDataIdx,
        )))),
        0x4B => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidRefIdx,
        )))),
        0x4C => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::ConstExprRequired,
        )))),
        0x4D => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::DupExportName,
        )))),
        0x4E => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::ImmutableGlobal,
        )))),
        0x4F => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidResultArity,
        )))),
        0x50 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::MultiTables,
        )))),
        0x51 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::MultiMemories,
        )))),
        0x52 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidLimit,
        )))),
        0x53 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidMemPages,
        )))),
        0x54 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidStartFunc,
        )))),
        0x55 => Err(Box::new(WasmEdgeError::Core(CoreError::Validation(
            CoreValidationError::InvalidLaneIdx,
        )))),

        // Instantiation phase
        0x60 => Err(Box::new(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::ModuleNameConflict,
        )))),
        0x61 => Err(Box::new(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::IncompatibleImportType,
        )))),
        0x62 => Err(Box::new(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::UnknownImport,
        )))),
        0x63 => Err(Box::new(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::DataSegDoesNotFit,
        )))),
        0x64 => Err(Box::new(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::ElemSegDoesNotFit,
        )))),

        // Execution phase
        0x80 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::WrongInstanceAddress,
        )))),
        0x81 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::WrongInstanceIndex,
        )))),
        0x82 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::InstrTypeMismatch,
        )))),
        0x83 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::FuncTypeMismatch,
        )))),
        0x84 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::DivideByZero,
        )))),
        0x85 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::IntegerOverflow,
        )))),
        0x86 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::InvalidConvToInt,
        )))),
        0x87 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::TableOutOfBounds,
        )))),
        0x88 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::MemoryOutOfBounds,
        )))),
        0x89 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::Unreachable,
        )))),
        0x8A => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::UninitializedElement,
        )))),
        0x8B => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::UndefinedElement,
        )))),
        0x8C => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::IndirectCallTypeMismatch,
        )))),
        0x8D => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::HostFuncFailed,
        )))),
        0x8E => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::RefTypeMismatch,
        )))),
        0x8F => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::UnalignedAtomicAccess,
        )))),
        0x90 => Err(Box::new(WasmEdgeError::Core(CoreError::Execution(
            CoreExecutionError::ExpectSharedMemory,
        )))),

        _ => panic!("unknown error code: {code}"),
    }
}

/// Loads plugins from default paths.
///
/// The default paths include:
///
/// * The path specified by the `WASMEDGE_PLUGIN_PATH` environment variable.
///
pub fn load_plugin_from_default_paths() {
    unsafe { ffi::WasmEdge_PluginLoadWithDefaultPaths() }
}

/// Returns the major version value.
pub fn version_major_value() -> u32 {
    unsafe { ffi::WasmEdge_VersionGetMajor() }
}

/// Returns the minor version value.
pub fn version_minor_value() -> u32 {
    unsafe { ffi::WasmEdge_VersionGetMinor() }
}

/// Returns the patch version value.
pub fn version_patch_value() -> u32 {
    unsafe { ffi::WasmEdge_VersionGetPatch() }
}

/// Returns the version string.
pub fn version_string() -> String {
    unsafe {
        CStr::from_ptr(ffi::WasmEdge_VersionGet())
            .to_string_lossy()
            .into_owned()
    }
}

// #[derive(Debug)]
// pub struct Driver {}
// impl Driver {
/// Triggers the WasmEdge AOT compiler tool
pub fn driver_aot_compiler<I, V>(args: I) -> i32
where
    I: IntoIterator<Item = V>,
    V: AsRef<str>,
{
    // create a vector of zero terminated strings
    let args = args
        .into_iter()
        .map(|arg| CString::new(arg.as_ref()).unwrap())
        .collect::<Vec<CString>>();

    // convert the strings to raw pointers
    let mut c_args = args
        .iter()
        .map(|arg| arg.as_ptr())
        .collect::<Vec<*const std::os::raw::c_char>>();

    unsafe {
        ffi::WasmEdge_Driver_Compiler(c_args.len() as std::os::raw::c_int, c_args.as_mut_ptr())
    }
}

/// Triggers the WasmEdge runtime tool
pub fn driver_runtime_tool<I, V>(args: I) -> i32
where
    I: IntoIterator<Item = V>,
    V: AsRef<str>,
{
    // create a vector of zero terminated strings
    let args = args
        .into_iter()
        .map(|arg| CString::new(arg.as_ref()).unwrap())
        .collect::<Vec<CString>>();

    // convert the strings to raw pointers
    let mut c_args = args
        .iter()
        .map(|arg| arg.as_ptr())
        .collect::<Vec<*const std::os::raw::c_char>>();

    unsafe { ffi::WasmEdge_Driver_Tool(c_args.len() as std::os::raw::c_int, c_args.as_mut_ptr()) }
}
