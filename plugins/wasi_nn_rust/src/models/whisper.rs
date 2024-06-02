use burn::config::Config;
use burn::module::Module;
use burn::record::{DefaultRecorder, Recorder};
use burn::tensor::backend::Backend;
use std::collections::HashMap;
use std::marker::PhantomData;
use std::process;
use strum::IntoEnumIterator;
use whisper_burn::model::Whisper as Model;
use whisper_burn::model::WhisperConfig as ModelConfig;
use whisper_burn::token::{Gpt2Tokenizer, Language};
use whisper_burn::transcribe::waveform_to_text;

pub struct GraphInner<B: Backend> {
    pub model: Model<B>,
    pub metadata: Vec<String>,
}

impl<B: Backend> GraphInner<B> {
    pub fn create(args: Vec<&str>, device: &B::Device) -> Self {
        if args.len() < 4 {
            eprintln!(
                "[WASI_NN] Invalid nn-preload model format. {:?} len < 4",
                args
            );
            process::exit(1);
        }
        let weights_path = args[0];
        let config_path = args[1];
        let config = match ModelConfig::load(config_path) {
            Ok(config) => config,
            Err(e) => {
                eprintln!("Failed to load whisper config: {}", e);
                process::exit(1);
            }
        };
        let recorder = DefaultRecorder::new().load(weights_path.into(), device);
        let model = recorder
            .map(|record| config.init(device).load_record(record))
            .unwrap();
        Self {
            model: model,
            metadata: args[2..].iter().map(|&s| s.to_string()).collect(),
        }
    }
    pub fn compute(&self, input: Vec<f32>) -> Vec<u8> {
        let tokenizer_path = &self.metadata[0].to_string();
        let lang_str = &self.metadata[1].to_string();
        let lang = match Language::iter().find(|lang| lang.as_str() == lang_str) {
            Some(lang) => lang,
            None => {
                eprintln!("Invalid language abbreviation: {}", lang_str);
                process::exit(1);
            }
        };
        let bpe = match Gpt2Tokenizer::new_with_path(tokenizer_path) {
            Ok(bpe) => bpe,
            Err(e) => {
                eprintln!("Failed to load tokenizer: {}", e);
                process::exit(1);
            }
        };
        let (text, _) = match waveform_to_text(&self.model, &bpe, lang, input, 16000) {
            Ok((text, tokens)) => (text, tokens),
            Err(e) => {
                eprintln!("Error during transcription: {}", e);
                process::exit(1);
            }
        };
        return text.into_bytes();
    }
}

pub const INPUT_DIM: usize = 2;

pub struct ContextInner<B: Backend> {
    pub inputs: HashMap<u32, Vec<f32>>,
    pub outputs: Vec<Vec<u8>>,
    _marker: PhantomData<B>,
}

impl<B: Backend> ContextInner<B> {
    pub fn new() -> Self {
        Self {
            inputs: HashMap::new(),
            outputs: Vec::new(),
            _marker: PhantomData,
        }
    }
    pub fn set_input(&mut self, key: u32, input: &[f32], _: [usize; INPUT_DIM]) {
        self.inputs.insert(key, input.to_vec());
    }
    pub fn get_output(&mut self, key: usize) -> &Vec<u8> {
        &self.outputs[key]
    }
}
