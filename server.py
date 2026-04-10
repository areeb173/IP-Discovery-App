from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import os
import sys
import urllib.request
import json
import threading
import uuid

# Determine base path — works both in dev and when frozen by PyInstaller
if getattr(sys, 'frozen', False):
    BASE_DIR = sys._MEIPASS
else:
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))

FRONTEND_DIST = os.path.join(BASE_DIR, "frontend", "dist")

app = Flask(__name__, static_folder=FRONTEND_DIST, static_url_path="")
CORS(app, supports_credentials=True, resources={r"/api/*": {
    "origins": "*",
    "methods": ["GET", "POST", "DELETE", "OPTIONS"],
    "allow_headers": ["Content-Type"]
}})

SUPPORTED_EXTENSIONS = {
    ".py", ".js", ".ts", ".java", ".cs",
    ".cpp", ".c", ".md", ".txt", ".json",
    ".yaml", ".yml"
}

MAX_FILE_SIZE = 5 * 1024 * 1024  # 5 MB

KEYWORDS_FILE = os.path.join(BASE_DIR, "keywords.txt")
REJECTED_FILE = os.path.join(BASE_DIR, "rejected_files.txt")
OLLAMA_URL = "http://localhost:11434/api/generate"

scan_progress = {}  # track progress per scan_id
scan_lock = threading.Lock()


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

def load_rejected():
    rejected = set()
    try:
        if os.path.exists(REJECTED_FILE):
            with open(REJECTED_FILE, "r", encoding="utf-8") as f:
                for line in f:
                    p = line.strip()
                    if p:
                        rejected.add(p)
    except Exception as e:
        print(f"Error loading rejected files: {e}")
    return rejected

def save_rejected(rejected):
    try:
        with open(REJECTED_FILE, "w", encoding="utf-8") as f:
            for p in sorted(rejected):
                f.write(p + "\n")
    except Exception as e:
        print(f"Error saving rejected files: {e}")

def query_ollama(prompt):
    data = {
        "model": "gemma3:4b",
        "prompt": prompt,
        "stream": False
    }
    try:
        req = urllib.request.Request(
            OLLAMA_URL,
            data=json.dumps(data).encode("utf-8"),
            headers={"Content-Type": "application/json"}
        )
        with urllib.request.urlopen(req, timeout=30) as response:
            result = json.loads(response.read().decode("utf-8"))
            return result.get("response", "").strip()
    except Exception as e:
        print(f"Error querying Ollama: {e}")
        return ""


def score_invention(text, matching_keywords):
    """Ask Ollama to score the text for IP potential. Returns dict with score, ip_type, summary, reasoning."""
    prompt = f"""You are evaluating source code or technical text for potential intellectual property.

Return ONLY valid JSON in this exact format:
{{"score": 0, "ip_type": "patent", "summary": "short summary", "reasoning": "short reason"}}

Rules:
- score is an integer 0-100 representing IP potential
- ip_type must be one of: patent, trade_secret, trademark, none
- Be conservative: generic algorithms, boilerplate, and standard patterns score low
- Only novel, non-obvious technical methods score above 40
- if this does not look like real technical IP, use score 0 and ip_type "none"

Keywords matched: {', '.join(matching_keywords)}

Text:
{text[:2000]}
"""
    response = query_ollama(prompt)
    try:
        start = response.find("{")
        end = response.rfind("}") + 1
        if start != -1 and end > start:
            return json.loads(response[start:end])
    except Exception:
        pass
    return {"score": 0, "ip_type": "none", "summary": "", "reasoning": "Could not parse model response"}


def extract_text_from_file(file_path):
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            return f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return ""


def extract_best_snippet(text, matching_keywords, max_chars=2000):
    """Find the densest region of keyword hits and return a larger surrounding block."""
    text_lower = text.lower()
    # Find position of each keyword hit
    positions = []
    for kw in matching_keywords:
        pos = text_lower.find(kw)
        if pos != -1:
            positions.append(pos)
    if not positions:
        return text[:max_chars]
    # Use the median position as the center to avoid outliers
    positions.sort()
    center = positions[len(positions) // 2]
    start = max(0, center - max_chars // 2)
    end = min(len(text), start + max_chars)
    # Try to snap to a newline boundary so we don't cut mid-line
    snap_start = text.rfind("\n", 0, start)
    if snap_start != -1 and start - snap_start < 200:
        start = snap_start + 1
    return text[start:end]


MIN_KEYWORD_MATCHES = 1   # require at least this many distinct keyword hits before calling Ollama
MIN_IP_SCORE = 40         # minimum score to flag a file as potential IP


def check_for_potential_ip(text, keywords):
    text_lower = text.lower()
    matching_keywords = [kw for kw in keywords if kw in text_lower]

    # Require at least MIN_KEYWORD_MATCHES distinct matches to reduce false positives
    if len(matching_keywords) < MIN_KEYWORD_MATCHES:
        return None

    snippet = extract_best_snippet(text, matching_keywords)
    score_data = score_invention(snippet, matching_keywords)

    if score_data.get("score", 0) >= MIN_IP_SCORE:
        return {"keywords": matching_keywords, "score_data": score_data}

    return None


def run_scan(scan_id, scan_path, keywords, rejected):
    """Run the scan in a background thread and store results in scan_progress."""
    try:
        parsed_files = 0
        skipped_large_files = 0
        potential_ip_files = []
        log = []

        for root, dirs, files in os.walk(scan_path):
            for file in files:
                ext = os.path.splitext(file)[1].lower()
                file_path = os.path.join(root, file)

                if ext not in SUPPORTED_EXTENSIONS:
                    continue

                # Skip rejected files
                if file_path in rejected:
                    continue

                try:
                    file_size = os.path.getsize(file_path)
                except Exception:
                    continue

                if file_size > MAX_FILE_SIZE:
                    skipped_large_files += 1
                    log.append(f"Skipped (too large): {file_path}")
                    with scan_lock:
                        scan_progress[scan_id]["log"] = list(log)
                    continue

                text = extract_text_from_file(file_path)
                if not text.strip():
                    continue

                parsed_files += 1
                log.append(f"Parsed: {file_path}")

                with scan_lock:
                    scan_progress[scan_id]["current_file"] = file_path
                    scan_progress[scan_id]["parsed_files"] = parsed_files
                    scan_progress[scan_id]["log"] = list(log)

                ip_result = check_for_potential_ip(text, keywords)
                if ip_result:
                    kws = ip_result["keywords"]
                    sd = ip_result["score_data"]
                    log.append(f"  ✓ Potential IP! Score:{sd.get('score')} Type:{sd.get('ip_type')} — {', '.join(kws)}")
                    potential_ip_files.append({
                        "file_path": file_path,
                        "keywords": kws,
                        "score": sd.get("score", 0),
                        "ip_type": sd.get("ip_type", "none"),
                        "summary": sd.get("summary", ""),
                        "reasoning": sd.get("reasoning", "")
                    })
                    with scan_lock:
                        scan_progress[scan_id]["log"] = list(log)

        with scan_lock:
            scan_progress[scan_id] = {
                "status": "complete",
                "parsed_files": parsed_files,
                "skipped_files": skipped_large_files,
                "current_file": "",
                "potential_ip_files": potential_ip_files,
                "log": log
            }
    except Exception as e:
        print(f"ERROR in scan thread {scan_id}: {e}", flush=True)
        import traceback
        traceback.print_exc()
        with scan_lock:
            scan_progress[scan_id] = {
                "status": "complete",
                "parsed_files": 0,
                "skipped_files": 0,
                "current_file": "",
                "potential_ip_files": [],
                "log": [f"Error during scan: {e}"]
            }


@app.route("/api/scan", methods=["OPTIONS"])
def scan_preflight():
    return "", 204
 
 
@app.route("/api/scan", methods=["POST"])
def start_scan():
    data = request.get_json()
    scan_path = data.get("path", "").strip()
 
    if not scan_path:
        return jsonify({"error": "No path provided"}), 400
    if not os.path.exists(scan_path):
        return jsonify({"error": "Directory does not exist"}), 400
    if not os.path.isdir(scan_path):
        return jsonify({"error": "Path is not a directory"}), 400
 
    keywords = load_keywords()
    if not keywords:
        return jsonify({"error": "No keywords loaded. Check keywords.txt"}), 500
 
    rejected = load_rejected()
    scan_id = str(uuid.uuid4())

    # Initialize progress BEFORE starting thread to avoid race condition
    with scan_lock:
        scan_progress[scan_id] = {
            "status": "running",
            "parsed_files": 0,
            "skipped_files": 0,
            "current_file": "",
            "potential_ip_files": [],
            "log": []
        }

    thread = threading.Thread(target=run_scan, args=(scan_id, scan_path, keywords, rejected), daemon=True)
    thread.start()

    return jsonify({"scan_id": scan_id})
 
 
@app.route("/api/scan/<scan_id>", methods=["GET"])
def get_scan_status(scan_id):
    with scan_lock:
        progress = scan_progress.get(scan_id)
 
    if progress is None:
        return jsonify({"error": "Scan not found"}), 404
 
    return jsonify(progress)
 
 
@app.route("/api/reject", methods=["POST"])
def reject_file():
    data = request.get_json()
    file_path = data.get("file_path", "").strip()
 
    if not file_path:
        return jsonify({"error": "No file path provided"}), 400
 
    rejected = load_rejected()
    rejected.add(file_path)
    save_rejected(rejected)
 
    return jsonify({"success": True, "rejected": file_path})
 
 
@app.route("/api/reject", methods=["GET"])
def get_rejected():
    rejected = load_rejected()
    return jsonify({"rejected": sorted(list(rejected))})
 
 
@app.route("/api/reject", methods=["DELETE"])
def unreject_file():
    data = request.get_json()
    file_path = data.get("file_path", "").strip()
 
    if not file_path:
        return jsonify({"error": "No file path provided"}), 400
 
    rejected = load_rejected()
    rejected.discard(file_path)
    save_rejected(rejected)
 
    return jsonify({"success": True, "unrejected": file_path})
 
 
@app.route("/api/health", methods=["GET"])
def health():
    return jsonify({"status": "ok"})
 
 
@app.route("/", defaults={"path": ""})
@app.route("/<path:path>")
def serve_frontend(path):
    if path and os.path.exists(os.path.join(FRONTEND_DIST, path)):
        return send_from_directory(FRONTEND_DIST, path)
    return send_from_directory(FRONTEND_DIST, "index.html")

if __name__ == "__main__":
    # Auto-open browser after short delay
    app.run(port=5000, debug=False, use_reloader=False)