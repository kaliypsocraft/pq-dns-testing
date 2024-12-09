#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Incorrect usage: ./benchmark.sh <size_of_packet>"
    exit 1
fi

make 

for i in $(seq 100 100 1000) $(seq 5000 250 7500); do
    # Run the dynamic version and capture output
    output=$(./arrf_dynamic $i)
    time_taken=$(echo "$output" | grep -o "Time taken for copy_section: [0-9.]* seconds" | awk '{print $5}')
    echo "Dynamic version time for input $i: $time_taken"

    # Run the static version and capture output
    output=$(./arrf_static $i)
    time_taken=$(echo "$output" | grep -o "Time taken for copy_section: [0-9.]* seconds" | awk '{print $5}')
    echo "Static version time for input $i: $time_taken"
done