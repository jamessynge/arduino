#!/bin/bash -e

if [[ -z "$1" ]]
then
  echo "Usage: $0 <name-of-sketch>"
  exit 1
fi

PARENT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
set -x

DIR="${PARENT}/$1"
if [[ -d "${DIR}" ]]
then
  echo "Already exists: $DIR"
  exit 1
fi

mkdir "${PARENT}/$1"

cat <<EOFEOF >"${DIR}/$1.ino"
// TODO($USER): Describe this sketch.

void setup() {
  // Setup serial, wait for it to be ready so that our logging messages can be
  // read.
  Serial.begin(9600);
  while (!Serial) {}

  // Additional one-time setup goes here.
}

void loop() {
  // Code to run repeatedly.
}
EOFEOF

A_SKETCH_DIR="${HOME}/Arduino/$1"
if [[ -e "${A_SKETCH_DIR}" ]]
then
  echo -n ""
else
  ln -s "${DIR}" "${A_SKETCH_DIR}"
fi
