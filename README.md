# Local Directory Parser

A tool for scanning local directories to identify files containing potential intellectual property or patentable ideas using AI-powered keyword matching and analysis.

## Quick Start

### Prerequisites

1. **Python 3.7+**
2. **Ollama** with the `gemma3:4b` model

### Running the Tool

1. Ensure Ollama is running
2. Run the script:
   ```bash
   python local_dir_parser.py
   ```
3. When prompted, enter the path to the directory you want to scan (e.g., `C:\Users\YourName\code\my_project`)

### Files Needed

You only need these files to test:
- `local_dir_parser.py` — The main script
- `keywords.txt` — Keyword list (one keyword per line)

**You do NOT need** installer files, build configuration, or test data directories.

## How It Works

1. Scans the target directory for supported file types (`.py`, `.js`, `.ts`, `.java`, `.cs`, `.cpp`, `.c`, `.md`, `.txt`, `.json`, `.yaml`, `.yml`)
2. For files containing keywords from `keywords.txt`, extracts a text snippet
3. Uses the local Ollama AI (gemma3:4b model) to determine if the content describes a patentable invention:
   - Locates the first matching keyword in the file
   - Extracts a 300-character snippet around the keyword (150 chars before, 150 after)
   - Sends the snippet to Ollama with the prompt: "Does this text describe an invention or patentable idea? Answer only 'yes' or 'no'."
   - Flags the file if Ollama responds with "yes"
4. Reports files with potential IP matches
