#!/bin/bash

# Check if an input file is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: ./test.sh <input_file>"
    echo "Example: ./test.sh case1.txt  (looks in tests/ folder)"
    exit 1
fi

INPUT_FILE=$1

# If the file doesn't exist directly, look inside tests/
if [ ! -f "$INPUT_FILE" ]; then
    if [ -f "tests/$INPUT_FILE" ]; then
        INPUT_FILE="tests/$INPUT_FILE"
    else
        echo "Error: Input file '$INPUT_FILE' not found (also checked tests/$INPUT_FILE)."
        exit 1
    fi
fi

# Derive a base name for output images (strip directory and extension)
BASENAME=$(basename "$INPUT_FILE" .txt)
mkdir -p output

# Compile the C++ program
echo "Compiling C++ source files..."
g++ -std=c++17 -O2 -o main geometry.cpp dcel.cpp sweep.cpp triangulate.cpp main.cpp
if [ $? -ne 0 ]; then
    echo "Compilation failed! Please check your source files."
    exit 1
fi
echo "Compilation successful."
echo "----------------------------------------"

# Check for python virtual environment
# if [ ! -d "venv" ]; then
#     echo "Creating Python virtual environment..."
#     python3 -m venv venv
#     source venv/bin/activate
#     pip install matplotlib
# else
#     source venv/bin/activate
# fi

# Execute the parsed binary with the provided input text file and pipe to visualizer
echo "Running computation and visualization..."
./main < "$INPUT_FILE" > temp_out.txt
cat temp_out.txt
python3 visualize.py temp_out.txt "output/${BASENAME}.png"
rm temp_out.txt
echo "----------------------------------------"
echo "Execution and visualization finished successfully."
