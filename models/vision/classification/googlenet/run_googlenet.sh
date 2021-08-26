#!/bin/bash
# RUN: %s %t.1

model_name="googlenet"
model_file="$MODELS_ROOT/vision/classification/$model_name/$model_name.onnx"
image_dir="$MODELS_ROOT/vision/test_images"
curr_dir=`dirname $0`
if [[ $# != 0 ]];then
  export TEST_TEMP_DIR=`dirname $1`
fi

# check if GPU is enabled or not
if [[ $TEST_WITH_GPU -eq 1 ]]; then
  echo "======== Testing with ODLA TensorRT ========"
  for i in 1 2 4 8 16 32 64
  do 
  python3 $curr_dir/../../invoke_halo.py --batch_size $i --model $model_file \
          --label-file $curr_dir/../1000_labels.txt --image-dir $image_dir \
          --odla tensorrt | tee $1
  done
# RUN: FileCheck --input-file %t.1 %s
else
	echo "This tests uses ODLA TensorRT"
fi

# CHECK: dog.jpg ==> "Samoyed, Samoyede",
# CHECK-NEXT: food.jpg ==> "ice cream, icecream",
# CHECK-NEXT: plane.jpg ==> "airliner",
# CHECK-NEXT: sport.jpg ==> "ski",
