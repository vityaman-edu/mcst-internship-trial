#!/bin/sh

set -o errexit

cd "$(dirname -- "$0")"

cat /dev/random | head -c 1024  > input-random-1.generated.txt
cat /dev/random | head -c 2048  > input-random-2.generated.txt
cat /dev/random | head -c 4096  > input-random-3.generated.txt
cat /dev/zero   | head -c 10000 > input-zero-1.generated.txt

APP=../../build/filehash
"$APP" input-random-1.generated.txt   > output-random-1.generated.txt
"$APP" input-random-2.generated.txt   > output-random-2.generated.txt
"$APP" input-random-3.generated.txt   > output-random-3.generated.txt
"$APP" input-zero-1.generated.txt     > output-zero-1.generated.txt
