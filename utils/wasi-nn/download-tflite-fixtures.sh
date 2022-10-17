TODIR=$1

FIXTURE=https://raw.githubusercontent.com/gusye1234/WasmEdge-WASINN-examples/demo-tflite-image/tflite-birds_v1-image
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/lite-model_aiy_vision_classifier_birds_V1_3.tflite ]; then
    curl -o $TODIR/lite-model_aiy_vision_classifier_birds_V1_3.tflite $FIXTURE/lite-model_aiy_vision_classifier_birds_V1_3.tflite
fi
if [ ! -f $TODIR/birdx224x224x3.rgb ]; then
    curl -o $TODIR/birdx224x224x3.rgb $FIXTURE/birdx224x224x3.rgb
fi
