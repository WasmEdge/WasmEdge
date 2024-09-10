use burn::tensor::backend::Backend;
use burn::tensor::Tensor;
use squeezenet_burn::model::squeezenet1::Model;
use std::collections::HashMap;

pub struct GraphInner<B: Backend> {
    pub model: Model<B>,
}

impl<B: Backend> GraphInner<B> {
    pub fn create(args: Vec<&str>, device: &B::Device) -> Self {
        let weights_path = args[0];
        Self {
            model: Model::from_file(weights_path, device),
        }
    }
    pub fn compute(&self, input: Tensor<B, INPUT_DIM>) -> Tensor<B, OUTPUT_DIM> {
        self.model.forward(input)
    }
}

pub const INPUT_DIM: usize = 4;
pub const OUTPUT_DIM: usize = 2;

pub struct ContextInner<B: Backend> {
    pub inputs: HashMap<u32, Tensor<B, INPUT_DIM>>,
    pub outputs: Vec<Tensor<B, OUTPUT_DIM>>,
}

impl<B: Backend> ContextInner<B> {
    pub fn new() -> Self {
        Self {
            inputs: HashMap::new(),
            outputs: Vec::new(),
        }
    }
    pub fn set_input(&mut self, key: u32, input: &[B::FloatElem], dimens: [usize; INPUT_DIM]) {
        let device = Default::default();
        let tensor = Tensor::<B, 1>::from_data(&*input, &device).reshape(dimens);
        self.inputs.insert(key, tensor);
    }
    pub fn get_output(&mut self, key: usize) -> Vec<<B as Backend>::FloatElem> {
        self.outputs[key].clone().into_data().value
    }
}
