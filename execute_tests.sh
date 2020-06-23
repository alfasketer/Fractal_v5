#!/bin/sh
mkdir output
for k in $(seq 1 5)
do
    FILENAME=output/$(echo output_$k)
    echo "" > $FILENAME
    for i in 1 2 4 8 16
    do
        for j in $(seq 1 32)
        do
            ./fractal.out -s 2000x2000 -g $i -t $j >> $FILENAME
        done
    done
done

tar -czf output.tar.gz ./output/