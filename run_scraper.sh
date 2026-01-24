#!/bin/bash
# Run script for Topo scraper - automatically activates venv

cd "$(dirname "$0")"

# Activate virtual environment
if [ -d "venv" ]; then
    source venv/bin/activate
else
    echo "Error: Virtual environment not found. Run ./setup.sh first."
    exit 1
fi

# Run the scraper
python -m scraper "$@"
