#!/bin/bash

if ! [ -x "$(command -v markdown-pdf)" ]; then
  echo 'Error: markdown-pdf is not installed.'
  echo 'Please install it with npm i -g markdown-pdf';
  echo 'If npm fails, try again using node version 8';
  exit 1
fi

if ! [ -x "$(command -v ots)" ]; then
  echo 'Error: ots (opentimestamps-client) is not installed.'
  exit 1
fi

if ! [ -x "$(command -v zip)" ]; then
  echo 'Error: zip is not installed.'
  exit 1
fi

rm 2016400141.zip
rm 2016400141.zip.ots
rm Report.pdf
markdown-pdf Readme.md -o Report.pdf

cp src/Makefile Makefile
cp src/project2.c project2.c

zip 2016400141.zip -r project2.c Makefile Report.pdf
ots stamp 2016400141.zip
# ots upgrade 2016400141.zip.ots
rm Makefile
rm project2.c
rm -r 2016400141
unzip 2016400141.zip -d 2016400141

echo "Zip 2016400141.zip is created, don't forget to check its contents"
echo "Also, don't forget to push the .zip and .ots to github"
echo "Also don't forget to send the .zip to moodle"
echo "And, press the extra final submit button on moodle";