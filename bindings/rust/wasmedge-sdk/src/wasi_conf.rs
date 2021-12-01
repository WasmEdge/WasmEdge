use crate::vm::Vm;

#[derive(Debug)]
pub struct WasiConf<'a> {
    pub vm: &'a Vm<'a>,
    pub args: Option<Vec<&'a str>>,
    pub envs: Option<Vec<&'a str>>,
    pub preopens: Option<Vec<&'a str>>,
}

impl<'a> WasiConf<'a> {
    pub fn new(vm: &'a Vm<'a>) -> Self {
        Self {
            vm,
            args: None,
            envs: None,
            preopens: None,
        }
    }

    pub fn with_args(self, args: Vec<&'a str>) -> Self {
        Self {
            args: Some(args),
            ..self
        }
    }

    pub fn with_envs(self, envs: Vec<&'a str>) -> Self {
        Self {
            envs: Some(envs),
            ..self
        }
    }

    pub fn with_preopens(self, preopens: Vec<&'a str>) -> Self {
        Self {
            preopens: Some(preopens),
            ..self
        }
    }

    pub fn build(&self) {
        if let Some(vm) = &self.vm.inner {
            vm.init_wasi_obj(
                self.args.as_ref(),
                self.envs.as_ref(),
                self.preopens.as_ref(),
            );
        }
    }
}
