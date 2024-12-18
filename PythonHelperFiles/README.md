## Python Helper Files

This directory contains various Python scripts and modules designed to assist with data logging, analysis, and communication with SCPI instruments.

### Contents

- SerialLogger
- SingleFileAnalysis

### Installation

To install the required dependencies, run:
```sh
pip install -r requirements.txt
```

## Usage

### SerialLogger.py

This script logs and parses serial data from a specified COM port into a log file for later analysis and visualization using the SingleFileAnalysis.py script.

Usage:
```sh
python SerialLogger.py <COM_PORT> <LOG_FILE_PATH> [BAUD_RATE] [TIMEOUT]
```

### Example:

```sh
python SerialLogger.py COM3 log/serial_log.txt 115200 1
```

### SingleFileAnalysis.py

This script analyzes and plots data from a log file.

Usage:

```sh
python SingleFileAnalysis.py <LOG_FILE_PATH>
```

### Example:

```sh
python SingleFileAnalysis.py log/serial_log.txt
```

