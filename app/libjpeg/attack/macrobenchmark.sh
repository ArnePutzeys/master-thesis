#!/bin/bash

# List of input images
input_images=("../img/muskox_color.jpg" "../img/birds.jpg" "../img/logo.jpg" "../img/SIGSAC_logo_308.jpg" "../img/Wapiti_from_Wagon_Trails.jpg") 

enclave_path=/home/arne/encl_without_aexnotify.so

num_runs=10

# Loop through each input image
for input_image in "${input_images[@]}"; do

    image_name=$(basename "$input_image")
    
    for ((i=1; i<=num_runs; i++)); do
        output_file="reconstruct_${image_name}_iteration_${i}.bmp"
        echo "Iteration $i for image $image_name"

        ./target/release/libjpeg_attack -o $output_file -i $input_image --color enclave -e $enclave_path

    done
done
