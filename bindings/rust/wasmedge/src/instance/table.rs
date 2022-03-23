use crate::{error::Result, types::Val, wasmedge, RefType};

#[derive(Debug)]
pub struct Table<'instance> {
    pub(crate) inner: wasmedge::Table,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
    pub(crate) _marker: std::marker::PhantomData<&'instance ()>,
}
impl<'instance> Table<'instance> {
    pub fn name(&self) -> Option<&str> {
        match &self.name {
            Some(name) => Some(name.as_ref()),
            None => None,
        }
    }

    pub fn mod_name(&self) -> Option<&str> {
        match &self.mod_name {
            Some(mod_name) => Some(mod_name.as_ref()),
            None => None,
        }
    }

    pub fn registered(&self) -> bool {
        self.mod_name.is_some()
    }

    /// Returns the underlying type of this table, including its element type as well as the maximum/minimum lower
    /// bounds.
    pub fn ty(&self) -> Result<TableType> {
        let ty = self.inner.ty()?;
        let limit = ty.limit();
        Ok(TableType {
            elem_ty: ty.elem_ty(),
            min: limit.start().to_owned(),
            max: Some(limit.end().to_owned()),
        })
    }

    /// Returns the current size of this table.
    pub fn capacity(&self) -> u32 {
        self.inner.capacity() as u32
    }

    /// Grows the size of this table by `size` more elements
    pub fn grow(&mut self, size: u32) -> Result<()> {
        self.inner.grow(size)?;
        Ok(())
    }

    /// Returns the table element at the `index`.
    pub fn get(&self, index: u32) -> Result<Val> {
        let value = self.inner.get_data(index)?;
        Ok(value.into())
    }

    /// Stores the `data` at the `index` of this table.
    pub fn set(&mut self, data: Val, index: u32) -> Result<()> {
        self.inner.set_data(data.into(), index)?;
        Ok(())
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TableType {
    elem_ty: RefType,
    min: u32,
    max: Option<u32>,
}
impl TableType {
    pub fn new(elem_ty: RefType, min: u32, max: Option<u32>) -> Self {
        Self { elem_ty, min, max }
    }

    pub fn elem_ty(&self) -> RefType {
        self.elem_ty
    }

    pub fn minimum(&self) -> u32 {
        self.min
    }

    pub fn maximum(&self) -> Option<u32> {
        self.max
    }

    pub fn to_raw(self) -> Result<wasmedge::TableType> {
        let min = self.minimum();
        let max = match self.maximum() {
            Some(max) => max,
            None => u32::MAX,
        };
        let raw = wasmedge::TableType::create(self.elem_ty(), min..=max)?;
        Ok(raw)
    }
}
impl From<wasmedge::TableType> for TableType {
    fn from(ty: wasmedge::TableType) -> Self {
        let limit = ty.limit();
        Self {
            elem_ty: ty.elem_ty(),
            min: limit.start().to_owned(),
            max: Some(limit.end().to_owned()),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        types::{FuncRef, Val},
        Executor, ImportModuleBuilder, RefType, SignatureBuilder, Statistics, Store, ValType,
        Value,
    };

    #[test]
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
    fn test_table_basic() {
        // create an ImportModule
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![ValType::I32; 2])
                    .with_returns(vec![ValType::I32])
                    .build(),
                Box::new(real_add),
            )
            .expect("failed to add host func")
            .with_table("table", TableType::new(RefType::FuncRef, 10, Some(20)))
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
        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the exported host function
        let result = instance.func("add");
        assert!(result.is_some());
        let mut host_func = result.unwrap();

        // get the exported table by name
        let result = instance.table("table");
        assert!(result.is_some());
        let mut table = result.unwrap();

        // check table
        assert!(table.name().is_some());
        assert_eq!(table.name().unwrap(), "table");
        assert!(table.mod_name().is_some());
        assert_eq!(table.mod_name().unwrap(), "extern");
        assert_eq!(table.capacity(), 10);
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
        } else {
            assert!(false);
        }

        // set value to table[0]
        let func_ref = FuncRef::new(&mut host_func);
        let result = table.set(Val::FuncRef(Some(func_ref)), 0);
        assert!(result.is_ok());
        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_some());
            let func_ref = func_ref.unwrap();
            let result = func_ref.as_func();
            assert!(result.is_some());
            let host_func = result.unwrap();
            // check the signature of the host function
            let result = host_func.signature();
            assert!(result.is_ok());
            let signature = result.unwrap();
            assert!(signature.args().is_some());
            assert_eq!(signature.args().unwrap(), [ValType::I32; 2]);
            assert!(signature.returns().is_some());
            assert_eq!(signature.returns().unwrap(), [ValType::I32]);
        } else {
            assert!(false);
        }

        let result = store.named_instance("extern");
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
            let result = func_ref.as_func();
            assert!(result.is_some());
            let host_func = result.unwrap();
            // check the signature of the host function
            let result = host_func.signature();
            assert!(result.is_ok());
            let signature = result.unwrap();
            assert!(signature.args().is_some());
            assert_eq!(signature.args().unwrap(), [ValType::I32; 2]);
            assert!(signature.returns().is_some());
            assert_eq!(signature.returns().unwrap(), [ValType::I32]);
        } else {
            assert!(false);
        }
    }

    fn real_add(inputs: Vec<Value>) -> std::result::Result<Vec<Value>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![Value::from_i32(c)])
    }
}
