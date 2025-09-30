#!/bin/bash

echo "Building with debug options..."

# Function to build
build_project() {
    # Clean build
    rm -rf build
    mkdir build
    cd build
    
    # Configure and build
    cmake "$@" ..
    make -j$(nproc)
    echo "Build completed!"
    echo "Run with: ./timer_load"
}

# Configure với debug options
case "$1" in
    "timer")
        echo "Enabling Timer debug only"
        build_project -DENABLE_TIMER_DEBUG=ON
        ;;
    "looper")
        echo "Enabling SLLooper debug only"
        build_project -DENABLE_SLLOOPER_DEBUG=ON
        ;;
    "queue")
        echo "Enabling EventQueue debug only"
        build_project -DENABLE_EventQueue_DEBUG=ON
        ;;
    "all")
        echo "Enabling ALL debug"
        build_project -DENABLE_ALL_DEBUG=ON
        ;;
    "combo")
        echo "Enabling Timer + SLLooper debug"
        build_project -DENABLE_TIMER_DEBUG=ON -DENABLE_SLLOOPER_DEBUG=ON
        ;;
    "production"|"prod")
        echo "Building production version (no debug)"
        build_project
        ;;
    *)
        echo "Usage: $0 {timer|looper|queue|all|combo|production}"
        echo ""
        echo "Examples:"
        echo "  $0 timer       - Enable Timer debug only"
        echo "  $0 looper      - Enable SLLooper debug only" 
        echo "  $0 queue       - Enable EventQueue debug only"
        echo "  $0 all         - Enable ALL debug"
        echo "  $0 combo       - Enable Timer + SLLooper debug"
        echo "  $0 production  - Production build (no debug)"
        exit 1
        ;;
esac