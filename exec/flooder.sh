#!/bin/sh

outpath="logfile"

if [[ -e "$outpath" ]]; then
    touch "$outpath"
fi
echo > "$outpath"
for ((i = 0; i < 5; i ++)); do
    curl -v -H "Host: migros.ch" http://127.0.0.1:8081/Gum/
done
