#!/bin/bash
src_dir="web/src"
web_dir="build/web"
if [ ! -e "$web_dir" ]
then
  mkdir ./build/web
  cp -r $src_dir/html/* $web_dir
  cp -r $src_dir/images $web_dir
fi

# Building js-file from sources
uglifyjs $src_dir/js/*.js -o $web_dir/arcui.js --beautify
