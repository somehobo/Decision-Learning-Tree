#!/bin/bash


make

./sequential_CART >> output/4_fold_sequential.txt <<< "4/n"

./sequential_CART >> output/7_fold_sequential.txt
    echo 7
    echo 5
    echo 10
./sequential_CART >> output/14_fold_sequential.txt
    echo 14
    echo 5
    echo 10
./sequential_CART >> output/28_fold_sequential.txt
    echo 28
    echo 5
    echo 10

mpirun -n 5 -ppn 4 -f c1_hosts ./parallel_CART >> output/4_fold_parallel.txt
    echo 5
    echo 10
mpirun -n 8 -ppn 4 -f c1_hosts ./parallel_CART >> output/7_fold_parallel.txt
    echo 5
    echo 10
mpirun -n 15 -ppn 4 -f c1_hosts ./parallel_CART >> output/14_fold_parallel.txt
    echo 5
    echo 10
mpirun -n 29 -ppn 4 -f c1_hosts ./parallel_CART >> output/28_fold_parallel.txt
    echo 5
    echo 10
