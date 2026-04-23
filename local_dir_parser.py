import os
import urllib.request
import json
import time
from datetime import datetime

SUPPORTED_EXTENSIONS = {
    ".py", ".js", ".ts", ".java", ".cs",
    ".cpp", ".c", ".md", ".txt", ".json",
    ".yaml", ".yml"
}

MAX_FILE_SIZE = 5 * 1024 * 1024  # 5 mb can be lowered/increased

KEYWORDS_FILE = "keywords.txt"
OLLAMA_URL = "http://localhost:11434/api/generate"
IDD_OUTPUT_FILE = "idd_autofill.md"

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


def generate_idd_draft(scan_path, potential_ip_files):
    if not potential_ip_files:
        return {
            "title": "No clear invention identified",
            "problem_statement": "No strong invention-like snippets were found in this scan.",
            "proposed_solution": "Run another scan with a richer keyword set and a narrower target directory.",
            "novelty_points": ["No novelty points extracted."],
            "implementation_highlights": ["No implementation highlights extracted."],
            "potential_claims": ["No potential claims identified."],
            "business_value": "Not enough evidence to estimate value.",
            "open_questions": ["Do we need additional technical source files or design docs?"],
            "recommended_next_steps": ["Refine keywords and rescan."],
        }

    top_findings = sorted(
        potential_ip_files,
        key=lambda item: item.get("score", 0),
        reverse=True,
    )[:8]

    findings_for_prompt = [
        {
            "file_path": item["file_path"],
            "keywords": item.get("keywords", []),
            "score": item.get("score", 0),
            "ip_type": item.get("ip_type", "none"),
            "summary": item.get("summary", ""),
            "reasoning": item.get("reasoning", ""),
            "snippet": item.get("snippet", "")[:450],
        }
        for item in top_findings
    ]

    prompt = f"""
You are helping complete section 3.1 of an invention disclosure document (IDD).
Use the scan findings below to auto-fill a concise draft.

Return ONLY valid JSON with this schema:
{{
  "title": "string",
  "problem_statement": "string",
  "proposed_solution": "string",
  "novelty_points": ["string"],
  "implementation_highlights": ["string"],
  "potential_claims": ["string"],
  "business_value": "string",
  "open_questions": ["string"],
  "recommended_next_steps": ["string"]
}}

Rules:
- Keep each bullet under 25 words.
- Be conservative; avoid legal certainty language.
- If evidence is weak, explicitly say assumptions are based on limited snippets.
- Use file evidence where possible.

Scan path: {scan_path}
Findings:
{json.dumps(findings_for_prompt, indent=2)}
"""

    response = query_ollama(prompt)
    try:
        start = response.find("{")
        end = response.rfind("}") + 1
        if start != -1 and end != -1:
            parsed = json.loads(response[start:end])
            return parsed
    except Exception as e:
        print(f"Error parsing IDD response: {e}")

    return {
        "title": "Potential invention concept from scan",
        "problem_statement": "Could not reliably parse LLM output for the IDD draft.",
        "proposed_solution": "Review the highest-scoring files manually and refine the technical concept.",
        "novelty_points": ["Draft generation failed due to invalid model output."],
        "implementation_highlights": ["Inspect scan results printed in console output."],
        "potential_claims": ["Potential claims require manual review."],
        "business_value": "Unknown without a validated concept description.",
        "open_questions": ["Should the prompt be tightened for structured JSON output?"],
        "recommended_next_steps": ["Re-run scan and retry IDD generation."],
    }


def _as_bullets(items):
    if not isinstance(items, list) or not items:
        return "- N/A"
    cleaned = [str(item).strip() for item in items if str(item).strip()]
    if not cleaned:
        return "- N/A"
    return "\n".join(f"- {item}" for item in cleaned)


def write_idd_template(scan_path, idd_data, potential_ip_files):
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    top_files = sorted(
        potential_ip_files,
        key=lambda item: item.get("score", 0),
        reverse=True
    )[:5]

    evidence_lines = []
    for item in top_files:
        evidence_lines.append(
            f"- `{item['file_path']}` (score: {item.get('score', 0)}, type: {item.get('ip_type', 'none')})\n"
            f"  - Keywords: {', '.join(item.get('keywords', []))}\n"
            f"  - Summary: {item.get('summary', '').strip() or 'N/A'}"
        )
    evidence_block = "\n".join(evidence_lines) if evidence_lines else "- No strong evidence captured."

    content = f"""# Invention Disclosure Draft (Auto-Filled)

## 3.1 Complete IDD Template

Generated: {now}
Scan path: `{scan_path}`

### Title
{idd_data.get("title", "Untitled invention concept")}

### Problem Statement
{idd_data.get("problem_statement", "N/A")}

### Proposed Solution
{idd_data.get("proposed_solution", "N/A")}

### Novelty Points
{_as_bullets(idd_data.get("novelty_points"))}

### Implementation Highlights
{_as_bullets(idd_data.get("implementation_highlights"))}

### Potential Claims (Draft)
{_as_bullets(idd_data.get("potential_claims"))}

### Business Value
{idd_data.get("business_value", "N/A")}

### Open Questions / Risks
{_as_bullets(idd_data.get("open_questions"))}

### Recommended Next Steps
{_as_bullets(idd_data.get("recommended_next_steps"))}

### Evidence from Scan Results
{evidence_block}

---
This draft is machine-generated from limited code snippets and is not legal advice.
"""

    try:
        with open(IDD_OUTPUT_FILE, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"\nAuto-filled IDD template written to: {IDD_OUTPUT_FILE}")
    except Exception as e:
        print(f"Error writing IDD template: {e}")

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

    return potential_ip_files

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

    potential_ip_files = scan_local_directory(scan_path, keywords)
    print("\nGenerating 3.1 Complete IDD template from scan results...")
    idd_data = generate_idd_draft(scan_path, potential_ip_files)
    write_idd_template(scan_path, idd_data, potential_ip_files)
    input("\nPress Enter to exit...")

if __name__ == "__main__":
    main()
