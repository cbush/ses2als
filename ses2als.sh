#!/bin/bash -e

DIR="`dirname $0`"

if [ -z "$2" ]; then
    echo "Usage: $0 <path/to/ses/file> <new project name>"
    exit 1
fi

mkdir "$2 Project"
rmdir "$2 Project"

cp -R "$DIR"/templates/project/ "$2 Project"
mkdir -p "$2 Project/Samples/Imported"

"$DIR"/bin/ses2als "$1" > "$2 Project/$2.xml"

SESDIR="`dirname $1`"
cp -R "$SESDIR/"*.wav "$2 Project/Samples/Imported"

pushd "$2 Project" > /dev/null

gzip "$2.xml"
mv "$2.xml.gz" "$2.als"

popd > /dev/null

echo "Created project $2 at `dirname $1`/$2 Project/"
