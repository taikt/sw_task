#!/bin/bash

# Default commit message
COMMIT_MSG="update"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -m|--message)
            COMMIT_MSG="$2"
            shift # past argument
            shift # past value
            ;;
        -h|--help)
            echo "Usage: $0 [-m|--message 'commit message']"
            echo "  -m, --message: Custom commit message (default: 'update')"
            echo "  -h, --help:    Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

# Cleanup
find . -name "*.exe" -type f -delete
find . -name "a.out" -type f -delete

# Git operations
git pull
git add -u .
git add *
git commit -m "$COMMIT_MSG"
git push

echo "✅ Committed with message: '$COMMIT_MSG'"