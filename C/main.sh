#!/bin/bash

[[ $# -eq 2 ]] || { echo "Usage: $0 filename number"; exit 1; }

mkdir -p word
cp $1 a.docx
unzip -p $1 word/document.xml | ./main $2 2>word/document.xml
zip -r a.docx word/document.xml

# xdg-open a.docx
