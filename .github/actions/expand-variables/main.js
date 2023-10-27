const aarch64 = 'aarch64';
const x86_64 = 'x86_64';

const archList = [{
    arch: x86_64,
    runner: 'ubuntu-latest',
    docker_tag: 'manylinux2014_x86_64',
}, {
    arch: aarch64,
    runner: 'linux-arm64',
    docker_tag: 'manylinux2014_aarch64',
}];

const pluginList = [{
    name: 'wasi_nn',
    bin: 'libwasmedgePluginWasiNN.so',
    testBin: 'wasiNNTests',
    features: [{
        name: 'tensorflowlite',
        options: '-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=TensorFlowLite',
        archList: [aarch64, x86_64],
    }, {
        name: 'wasi_nn-ggml',
        options: '-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=GGML -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS=OFF',
        archList: [aarch64, x86_64],
    }, {
        name: 'wasi_nn-pytorch',
        options: '-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=PyTorch',
        archList: [x86_64],
    }],
}];

let expandVariables = () => {
    let ret = archList.map((arch) => {
        arch.plugins = pluginList.flatMap((plugin) => {
            if (plugin.hasOwnProperty('features'))
                return plugin.features
                    .filter((ft) => ft.archList.includes(arch.arch))
                    .map((ft) => ({
                        plugin: [plugin.name, ft.name].join('-'),
                        bin: plugin.bin,
                        dir: plugin.name,
                        testBin: plugin.testBin,
                        options: ft.options,
                    }));
            if (plugin.archList.includes(arch.arch))
                return {
                    plugin: plugin.name,
                    bin: plugin.bin,
                    dir: plugin.name,
                    testBin: plugin.testBin,
                    options: plugin.options,
                };
            return [];
        })
        return arch;
    });
    return ret;
};

const core = require('@actions/core');

try {
    output = JSON.stringify(expandVariables());
    core.setOutput('matrix', output);
    console.log(`matrix: ${output}`);
} catch (err) {
    core.setFailed(err.message);
}
