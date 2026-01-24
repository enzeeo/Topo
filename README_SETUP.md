# Setup Instructions

## The Problem
The error `ModuleNotFoundError: No module named 'yfinance'` occurs because:
1. **You're not using the virtual environment** - Python is running from the base conda environment
2. **Dependencies are installed in venv** - All packages are in `./venv/`, not in the base environment

## The Solution

### Option 1: Activate venv before running (Recommended)
```bash
# Activate the virtual environment
source venv/bin/activate

# Now run the scraper
python -m scraper
```

### Option 2: Use the setup script
```bash
# Run the setup script (installs deps if needed)
./setup.sh

# Then activate and run
source venv/bin/activate
python -m scraper
```

### Option 3: Use conda (if you prefer)
```bash
# Create conda environment
conda create -n topo python=3.10
conda activate topo

# Install dependencies
pip install -r requirements.txt

# Run scraper
python -m scraper
```

## Verify Installation
```bash
source venv/bin/activate
python -c "import yfinance; import pandas; import pyarrow; print('✓ All dependencies installed!')"
```

## Common Issues

1. **"ModuleNotFoundError"** → Activate venv first: `source venv/bin/activate`
2. **Network errors during install** → Check internet connection, then run `pip install -r requirements.txt` again
3. **Permission errors** → Use `pip install --user -r requirements.txt` or fix pip cache permissions
