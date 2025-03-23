#!/bin/bash

num_runs=100


for ((i=1; i<=num_runs; i++)); do
    output_file="reconstruct_color_${image_name}_iteration_${i}.bmp"
    echo "---------------------------------------------------------------------------------------"
    echo "Iteration $i"
    echo "---------------------------------------------------------------------------------------"

    ./app

done

