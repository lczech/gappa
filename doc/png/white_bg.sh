#!/bin/bash

for img in `ls *.png` ; do
    echo $img
    convert $img -background white -alpha remove -alpha off $img
done 
