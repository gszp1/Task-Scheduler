#!/bin/bash

file_path="remover"

# Check if the file does not exist
if [ ! -f "$file_path" ]; then
    gcc queue_remover.c -lrt -o remover
fi

# Run the program (assuming it's in the current directory)
./remover