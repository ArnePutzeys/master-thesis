#!/bin/bash

# List of input images
input_images=("../img/testimg-gray.jpg" "../img/birds-gray.jpg" "../img/logo-gray.jpg" "../img/SIGSAC_logo_308-gray.jpg" "../img/Wapiti_from_Wagon_Trails-gray.jpg") 

enclave_path=/home/arne/encl_without_aexnotify.so

num_runs=10

# Loop through each input image
for input_image in "${input_images[@]}"; do

    image_name=$(basename "$input_image")
    
    for ((i=1; i<=num_runs; i++)); do
        output_file="reconstruct_${image_name}_iteration_${i}.bmp"
        echo "Iteration $i for image $image_name"

        ./target/release/libjpeg_attack -o $output_file -i $input_image enclave -e $enclave_path

    done
done
