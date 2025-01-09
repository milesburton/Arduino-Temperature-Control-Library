#!/bin/bash

# Set error handling
set -e

# Function to display usage
show_usage() {
    echo "Usage: ./build.sh [option]"
    echo "Options:"
    echo "  build    - Only compile library and examples"
    echo "  test     - Only compile and run tests"
    echo "  all      - Build everything and run tests (default)"
}

# Function to compile for a specific board
compile_for_board() {
    local fqbn=$1
    local sketch=$2
    echo "📦 Compiling $sketch for $fqbn..."
    arduino-cli compile --fqbn $fqbn "$sketch" --library .
}

# Function to build library and examples
build() {
    echo "🔨 Building library and examples..."
    
    # Compile all examples
    echo "🔍 Compiling examples..."
    for example in examples/*/*.ino; do
        if [ -f "$example" ]; then
            echo "Building example: $example"
            compile_for_board "arduino:avr:uno" "$example"
        fi
    done
}

# Function to run tests
run_tests() {
    echo "🧪 Running tests..."
    for test in test/*/*.ino; do
        if [ -f "$test" ]; then
            echo "Running test: $test"
            compile_for_board "arduino:avr:uno" "$test"
        fi
    done
}

# Main execution
case "${1:-all}" in
    "build")
        build
        ;;
    "test")
        run_tests
        ;;
    "all")
        build
        run_tests
        ;;
    *)
        show_usage
        exit 1
        ;;
esac

echo "✅ Process completed!"