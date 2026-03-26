import os
import urllib.request
import json
import time

SUPPORTED_EXTENSIONS = {
    ".py", ".js", ".ts", ".java", ".cs",
    ".cpp", ".c", ".md", ".txt", ".json",
    ".yaml", ".yml"
}

MAX_FILE_SIZE = 5 * 1024 * 1024  # 5 mb can be lowered/increased

KEYWORDS_FILE = "keywords.txt"
OLLAMA_URL = "http://localhost:11434/api/generate"

# read keyword list from file into a set for quick lookup
def load_keywords():
    keywords = set()
    try:
        with open(KEYWORDS_FILE, "r", encoding="utf-8") as f:
            for line in f:
                word = line.strip().lower()
                if word:
                    keywords.add(word)
    except Exception as e:
        print(f"Error loading keywords: {e}")
    return keywords

# send a prompt to Ollama api and return its text response
def query_ollama(prompt):
    # attempt to call the Ollama server; return empty string on failure
    data = {
        "model": "gemma3:4b",
        "prompt": prompt,
        "stream": False
    }
    try:
        req = urllib.request.Request(OLLAMA_URL, data=json.dumps(data).encode('utf-8'), headers={'Content-Type': 'application/json'})
        with urllib.request.urlopen(req) as response:
            if response.status == 404:
                print("Ollama server returned 404. Ensure the API path and server are correct.")
                return ""
            result = json.loads(response.read().decode('utf-8'))
            return result.get('response', '').strip()
    except urllib.error.HTTPError as e:
        if e.code == 404:
            print("Ollama endpoint not found (404). Is the server running and model available?")
        else:
            print(f"HTTP error from Ollama: {e.code} - {e.reason}")
        return ""
    except Exception as e:
        print(f"Error querying Ollama: {e}")
        return ""

# ask the LLM whether the text seems like a patentable idea
def is_invention_like(text):
    prompt = f"Does this text describe an invention or patentable idea? Answer only 'yes' or 'no'.\n\nText:\n{text[:2000]}"  # Limit text to 2000 chars
    response = query_ollama(prompt)
    return response.lower().startswith('yes')

def score_invention(text, matching_keywords):
    prompt = f"""
You are evaluating text for potential intellectual property.

Return ONLY valid JSON in this format:
{{
  "score": 0,
  "ip_type": "patent",
  "summary": "short summary here",
  "reasoning": "short reason here"
}}

Rules:
- score must be an integer from 0 to 100
- ip_type must be one of: patent, trade_secret, trademark, none
- be conservative
- if this does not look like real technical IP, use a low score and ip_type "none"

Keywords found: {", ".join(matching_keywords)}

Text:
{text[:2000]}
"""
    response = query_ollama(prompt)

    try:
        start = response.find("{")
        end = response.rfind("}") + 1
        if start != -1 and end != -1:
            return json.loads(response[start:end])
    except Exception as e:
        print(f"Error parsing score response: {e}")

    return {
        "score": 0,
        "ip_type": "none",
        "summary": "Could not parse model response",
        "reasoning": "Invalid JSON from Ollama"
    }

# read and return contents of a file, ignoring errors
def extract_text_from_file(file_path):
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            return f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return ""

# determine if text contains keywords and is judged invention-like
def check_for_potential_ip(text, keywords):
    text_lower = text.lower()
    matching_keywords = [kw for kw in keywords if kw in text_lower]

    if matching_keywords:
        first_kw = matching_keywords[0]
        kw_pos = text_lower.find(first_kw)

        start = max(0, kw_pos - 150)
        snippet = text[start:start + 300]

        if is_invention_like(snippet):
            score_data = score_invention(snippet, matching_keywords)
            return {
                "keywords": matching_keywords,
                "snippet": snippet,
                "score_data": score_data
            }

    return None

# walk through directory tree, analyze each supported file
def scan_local_directory(scan_path, keywords):
    parsed_files = 0
    skipped_large_files = 0
    potential_ip_files = []

    print(f"\nScanning directory: {scan_path}\n")
    start_time = time.time()

    for root, dirs, files in os.walk(scan_path):
        for file in files:
            ext = os.path.splitext(file)[1].lower()
            file_path = os.path.join(root, file)

            if ext in SUPPORTED_EXTENSIONS:
                try:
                    file_size = os.path.getsize(file_path)
                except Exception:
                    continue

                if file_size > MAX_FILE_SIZE:
                    skipped_large_files += 1
                    print(f"Skipped (too large): {file_path}")
                    continue

                text = extract_text_from_file(file_path)

                if text.strip():
                    parsed_files += 1
                    print(f"Parsed: {file_path}")

                    result = check_for_potential_ip(text, keywords)

                    if result:
                        print("Potential IP found!")
                        print(f"Matching keywords: {', '.join(result['keywords'])}")
                        print(f"Score: {result['score_data'].get('score', 0)}")
                        print(f"Type: {result['score_data'].get('ip_type', 'none')}")
                        print(f"Summary: {result['score_data'].get('summary', '')}")
                        print()

                        potential_ip_files.append({
                            "file_path": file_path,
                            "keywords": result["keywords"],
                            "snippet": result["snippet"],
                            "score": result["score_data"].get("score", 0),
                            "ip_type": result["score_data"].get("ip_type", "none"),
                            "summary": result["score_data"].get("summary", ""),
                            "reasoning": result["score_data"].get("reasoning", "")
                        })

    print(f"Total Parsed Files: {parsed_files}")
    print(f"Skipped Large Files: {skipped_large_files}")
    print(f"Potential IP Files: {len(potential_ip_files)}")

    end_time = time.time()
    total_time = end_time - start_time

    print(f"\nTotal runtime: {total_time:.2f} seconds")

    if parsed_files > 0:
        avg_time = total_time / parsed_files
        print(f"Average time per file: {avg_time:.4f} seconds")

    if potential_ip_files:
        print("\nPotential IP Files:")
        for item in potential_ip_files:
            print(f"- {item['file_path']}")
            print(f"  Keywords: {', '.join(item['keywords'])}")
            print(f"  Score: {item['score']}")
            print(f"  Type: {item['ip_type']}")
            print(f"  Summary: {item['summary']}")
            print()

def main():
    keywords = load_keywords()
    if not keywords:
        print("No keywords loaded. Please check keywords.txt")
        input("Press Enter to exit...")
        return

    scan_path = input("Enter directory path to scan: ").strip()

    if not os.path.exists(scan_path):
        print("Error: Directory does not exist.")
        input("Press Enter to exit...")
        return

    if not os.path.isdir(scan_path):
        print("Error: Path is not a directory.")
        input("Press Enter to exit...")
        return

    scan_local_directory(scan_path, keywords)
    input("\nPress Enter to exit...")

if __name__ == "__main__":
    main()
