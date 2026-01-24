#!/bin/bash
# Setup script for Topo scraper

echo "Setting up Topo scraper environment..."

# Activate virtual environment
if [ -d "venv" ]; then
    echo "Activating virtual environment..."
    source venv/bin/activate
else
    echo "Creating virtual environment..."
    python3 -m venv venv
    source venv/bin/activate
fi

# Upgrade pip
echo "Upgrading pip..."
pip install --upgrade pip

# Install dependencies
echo "Installing dependencies from requirements.txt..."
pip install -r requirements.txt

# Verify installation
echo ""
echo "Verifying installation..."
python -c "import yfinance; import pandas; import pyarrow; import numpy; import dateutil; print('✓ All dependencies installed successfully!')"

echo ""
echo "Setup complete! To use the scraper:"
echo "  1. Activate venv: source venv/bin/activate"
echo "  2. Run scraper: python -m scraper"
