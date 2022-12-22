use crate::{types::Val, WasmEdgeResult};
use wasmedge_sys as sys;
use wasmedge_types::TableType;

/// Defines a table storing the references to host functions or external objects.
#[derive(Debug)]
pub struct Table {
    pub(crate) inner: sys::Table,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Table {
    /// Creates a new wasm table instance with the given type.
    ///
    /// # Argument
    ///
    /// * `ty` - The type of the table instance to be created.
    ///
    /// # Error
    ///
    /// If fail to create the table instance, then an error is returned.
    pub fn new(ty: TableType) -> WasmEdgeResult<Self> {
        let inner = sys::Table::create(&ty.into())?;
        Ok(Self {
            inner,
            name: None,
            mod_name: None,
        })
    }

    /// Returns the exported name of this [Table].
    ///
    /// Notice that this field is meaningful only if this table is used as an exported instance.
    pub fn name(&self) -> Option<&str> {
        match &self.name {
            Some(name) => Some(name.as_ref()),
            None => None,
        }
    }

    /// Returns the name of the [module instance](crate::Instance) from which this [Table] exports.
    ///
    /// Notice that this field is meaningful only if this table is used as an exported instance.
    pub fn mod_name(&self) -> Option<&str> {
        match &self.mod_name {
            Some(mod_name) => Some(mod_name.as_ref()),
            None => None,
        }
    }

    /// Returns the type of this table.
    ///
    /// # Error
    ///
    /// If fail to get the type of this table, then an error is returned.
    pub fn ty(&self) -> WasmEdgeResult<TableType> {
        let ty = self.inner.ty()?;
        Ok(ty.into())
    }

    /// Returns the size of this [Table].
    pub fn size(&self) -> u32 {
        self.inner.capacity() as u32
    }

    /// Grows the size of this table by `delta`, initializating the elements with the provided init value if `init` is given.
    ///
    /// # Arguments
    ///
    /// * `delta` - the number of elements to grow the table by.
    ///
    /// # Error
    ///
    /// If fail to grow the table, then an error is returned.
    pub fn grow(&mut self, delta: u32, init: Option<Val>) -> WasmEdgeResult<u32> {
        // get the current size
        let original_size = self.size();
        // grow the table by delta
        self.inner.grow(delta)?;
        // initialize the new elements
        if let Some(init) = init {
            for idx in original_size..original_size + delta {
                self.inner.set_data(init.clone().into(), idx)?;
            }
        }
        Ok(original_size)
    }

    /// Returns the table element at the `index`.
    ///
    /// # Argument
    ///
    /// * `index` - the index of the table element to get.
    ///
    /// # Error
    ///
    /// If fail to get the table element, then an error is returned.
    pub fn get(&self, index: u32) -> WasmEdgeResult<Val> {
        let value = self.inner.get_data(index)?;
        Ok(value.into())
    }

    /// Stores the `data` at the `index` of this table.
    ///
    /// # Arguments
    ///
    /// * `index` - the index of the table element to store.
    ///
    /// * `data` - the data to store at the `index` of this table.
    ///
    ///
    /// # Error
    ///
    /// If fail to store the data, then an error is returned.
    pub fn set(&mut self, index: u32, data: Val) -> WasmEdgeResult<()> {
        self.inner.set_data(data.into(), index)?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        error::HostFuncError,
        types::Val,
        CallingFrame, Executor, ImportObjectBuilder, RefType, Statistics, Store, ValType,
        WasmValue,
    };

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_table_type() {
        // create a TableType instance
        let ty = TableType::new(RefType::FuncRef, 10, Some(20));

        // check element type
        assert_eq!(ty.elem_ty(), RefType::FuncRef);
        // check minimum
        assert_eq!(ty.minimum(), 10);
        // check maximum
        assert_eq!(ty.maximum(), Some(20));
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_table_basic() {
        // create a table instance
        let result = Table::new(TableType::new(RefType::FuncRef, 10, Some(20)));
        assert!(result.is_ok());
        let table = result.unwrap();

        // create an import object
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", real_add)
            .expect("failed to add host func")
            .with_table("table", table)
            .expect("failed to add table")
            .build("extern");
        assert!(result.is_ok());
        let import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        // register the import module
        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        // get the module instance by name
        let result = store.module_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the exported host function
        let result = instance.func("add");
        assert!(result.is_some());
        let host_func = result.unwrap();

        // get the exported table by name
        let result = instance.table("table");
        assert!(result.is_some());
        let mut table = result.unwrap();

        // check table
        assert!(table.name().is_some());
        assert_eq!(table.name().unwrap(), "table");
        assert!(table.mod_name().is_some());
        assert_eq!(table.mod_name().unwrap(), "extern");
        assert_eq!(table.size(), 10);
        let result = table.ty();
        assert!(result.is_ok());

        // check table type
        let ty = result.unwrap();
        assert_eq!(ty.elem_ty(), RefType::FuncRef);
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum(), Some(20));

        // get value from table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_none());
        }

        // set value to table[0]
        let func_ref = host_func.as_ref();
        let result = table.set(0, Val::FuncRef(Some(func_ref)));
        assert!(result.is_ok());
        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_some());
            let func_ref = func_ref.unwrap();
            // check the signature of the host function
            let result = func_ref.ty();
            assert!(result.is_ok());
            let func_ty = result.unwrap();
            assert!(func_ty.args().is_some());
            assert_eq!(func_ty.args().unwrap(), [ValType::I32; 2]);
            assert!(func_ty.returns().is_some());
            assert_eq!(func_ty.returns().unwrap(), [ValType::I32]);
        }

        let result = store.module_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        let result = instance.table("table");
        assert!(result.is_some());
        let table = result.unwrap();

        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_some());
            let func_ref = func_ref.unwrap();
            let result = func_ref.ty();
            assert!(result.is_ok());
            let func_ty = result.unwrap();
            assert!(func_ty.args().is_some());
            assert_eq!(func_ty.args().unwrap(), [ValType::I32; 2]);
            assert!(func_ty.returns().is_some());
            assert_eq!(func_ty.returns().unwrap(), [ValType::I32]);
        }
    }

    fn real_add(
        _frame: CallingFrame,
        inputs: Vec<WasmValue>,
    ) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
        if inputs.len() != 2 {
            return Err(HostFuncError::User(1));
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(HostFuncError::User(2));
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(HostFuncError::User(3));
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
