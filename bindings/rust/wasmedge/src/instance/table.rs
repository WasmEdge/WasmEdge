use crate::{error::Result, wasmedge, RefType, Value};

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

    /// Returns the table element value at `index`.
    pub fn get(&self, index: u32) -> Result<Value> {
        let value = self.inner.get_data(index)?;
        Ok(value)
    }

    /// Writes the `data` provided into `index` within this table.
    pub fn set(&mut self, data: Value, index: u32) -> Result<()> {
        self.inner.set_data(data, index)?;
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
        Executor, ImportModuleBuilder, RefType, Statistics, Store,
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
    fn test_table() {
        // create an ImportModule
        let result = ImportModuleBuilder::new()
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
        let value = result.unwrap();
        assert!(value.func_idx().is_none());

        // set value to table[0]
        let result = table.set(Value::from_func_ref(5), 0);
        assert!(result.is_ok());
        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        let value = result.unwrap();
        assert!(value.func_idx().is_some());
        assert_eq!(value.func_idx().unwrap(), 5);

        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        let result = instance.table("table");
        assert!(result.is_some());
        let table = result.unwrap();

        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        let value = result.unwrap();
        assert!(value.func_idx().is_some());
        assert_eq!(value.func_idx().unwrap(), 5);
    }
}
