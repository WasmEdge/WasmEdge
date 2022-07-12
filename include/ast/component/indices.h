
#include <variant>
#include <map>
#include <iostream>
#include <optional>
#include <string>
#include "common/enum_types.h"

// These indices are used during compile time only when we're translating a
// component. The actual indices are not persisted beyond the
// compile phase to when we're actually working with the component at
// runtime.

/// Index within a component's component type index.
struct ComponentTypeIndex {
    int Idx;
};

/// Index within a component's module index.
struct ModuleIndex {
    int Idx;
};

/// Index within a component's component index.
struct ComponentIndex {
    int Idx;
};

/// Index within a component's module instance index.
struct ModuleInstanceIndex {
    int Idx;
};

/// Index within a component's component instance index.
struct ComponentInstanceIndex {
    int Idx;
};

/// Index within a component's component function index.
struct ComponentFunctionIndex {
    int Idx;
};

/// Index pointing to a component's type (exports/imports with
/// component-model types)
struct TypeComponentIndex {
    int Idx;
};

/// Index pointing to a component instance's type (exports with
/// component-model types, no imports)
struct TypeComponentInstanceIndex {
    int Idx;
};

 /// Index pointing to a core wasm module's type (exports/imports with
/// core wasm types)
struct TypeModuleIndex {
    int Idx;
};

/// Index pointing to a component model function type with arguments/result
/// as interface types.
struct TypeFuncIndex {
    int Idx;
};

 /// Index pointing to an interface type, used for recursive types such as
/// `List<T>`.
struct TypeInterfaceIndex {
    int Idx;
};

/// Index pointing to a record type in the component model.
struct TypeRecordIndex {
    int Idx;
};

/// Index pointing to a variant type in the component model.
struct TypeVariantIndex {
    int Idx;
};

/// Index pointing to a tuple type in the component model.
struct TypeTupleIndex {
    int Idx;
};

/// Index pointing to a flags type in the component model.
struct TypeFlagsIndex {
    int Idx;
};

/// Index pointing to an enum type in the component model.
struct TypeEnumIndex {
    int Idx;
};

/// Index pointing to a union type in the component model.
struct TypeUnionIndex {
    int Idx;
};

/// Index pointing to an expected type in the component model
struct TypeExpectedIndex {
    int Idx;
};

// Index types used to identify modules and components during compliation.

/// Index into a "closed over variables" list for components used to
/// implement outer aliases.
struct ModuleUpVarIndex {
    int Idx;
};

 /// Same as `ModuleUpvarIndex` but for components.
struct ComponentUpVarIndex {
    int Idx;
};

/// Index into the global list of modules found within an entire component.
struct StaticModuleIndex {
    int Idx;
};

/// Same as `StaticModuleIndex` but for components.
struct StaticComponentIndex {
    int Idx;
};

// These indices are actually used at runtime when managing a component at
// this time.

/// Index that represents a core wasm instance created at runtime.
///
/// This is used to keep track of when instances are created and is able to
/// refer back to previously created instances for exports and such.
struct RuntimeInstanceIndex {
    int Idx;
};

/// Same as `RuntimeInstanceIndex` but tracks component instances instead.
struct RuntimeComponentInstanceIndex {
    int Idx;
};

/// Used to index imports into a `Component`
struct ImportIndex {
    int Idx;
};

/// Index that represents a leaf item imported into a component
struct RuntimeImportIndex {
    int Idx;
};

/// Index that represents a lowered host function and is used to represent
/// host function lowerings with options and such.
struct LoweredIndex {
    int Idx;
};

/// Index representing a linear memory extracted from a wasm instance.
struct RuntimeMemoryIndex {
    int Idx;
};

// Same as `RuntimeMemoryIndex` except for the `realloc` function.
struct RuntimeReallocIndex {
    int Idx;
};

/// Same as `RuntimeMemoryIndex` except for the `post-return` function.
struct RuntimePostReturnIndex {
    int Idx;
};

/// Index that represents an exported module from a component since that's
/// currently the only use for saving the entire module state at runtime.
struct RuntimeModuleIndex {
    int Idx;
};


/// A component and its type.
struct Component {
 TypeComponentIndex Idx;
};

/// An instance of a component.
struct ComponentInstance {
    TypeComponentInstanceIndex Idx;
};

/// A component function, not to be confused with a core wasm function.
struct ComponentFunc {
    TypeFuncIndex Idx;
};

/// An interface type.
struct Interface {
    InterfaceType Idx;
};

/// A core wasm module and its type.
struct Module {
    TypeModuleIndex Idx;
};

/// These types are what's available for import and export in components.
using TypeDef = std::variant<Component,ComponentInstance,ComponentFunc,Interface,Module,CoreFunc>;

/// The type of a component in the component model.
struct TypeComponent {
    std::map<std::string, TypeDef> imports;
    std::map<std::string, TypeDef> exports;
};

/// Component instances only have exports of types in the component model.
struct TypeComponentInstance {
    std::map<std::string, TypeDef> exports;
};

/// A component function type in the component model.
struct TypeFunc {
    std::vector<std::pair<std::optional<std::string>, InterfaceType>> params;
    InterfaceType results;
};