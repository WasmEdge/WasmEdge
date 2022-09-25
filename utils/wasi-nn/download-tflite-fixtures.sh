TODIR=$1

FIXTURE_EX=https://raw.githubusercontent.com/second-state/wasm-learning/master/rust/birds_v1
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/lite-model_aiy_vision_classifier_birds_V1_3.tflite ]; then
    curl -o $TODIR/lite-model_aiy_vision_classifier_birds_V1_3.tflite $FIXTURE_EX/lite-model_aiy_vision_classifier_birds_V1_3.tflite
fi

FIXTURE=https://raw.githubusercontent.com/gusye1234/WasmEdge-WASINN-examples/demo-tflite-image/tflite-birds_v1-image
if [ ! -f $TODIR/birdx224x224x3.rgb ]; then
    curl -o $TODIR/birdx224x224x3.rgb $FIXTURE/birdx224x224x3.rgb
fi
