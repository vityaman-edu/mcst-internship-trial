#!/bin/sh

set -o errexit

cd "$(dirname -- "$0")"

APP=../../build/filehash
OUT_PATH=../../build
STDOUT_FILE=$OUT_PATH/output.txt

echo "Validating samples..."
for filename in input-*\.txt; do
    SAMPLE_NAME=${filename#*-}
    SAMPLE_NAME=${SAMPLE_NAME%.txt*}

    INPUT_FILE=input-$SAMPLE_NAME.txt
    OUTPUT_FILE=output-$SAMPLE_NAME.txt

    $APP $INPUT_FILE > $STDOUT_FILE

    diff --color $OUTPUT_FILE $STDOUT_FILE
    echo "Sample is OK: '$SAMPLE_NAME'!"
done
