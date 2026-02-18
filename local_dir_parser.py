import os

SUPPORTED_EXTENSIONS = {
    ".py", ".js", ".ts", ".java", ".cs",
    ".cpp", ".c", ".md", ".txt", ".json",
    ".yaml", ".yml"
}

MAX_FILE_SIZE = 5 * 1024 * 1024  # 5 mb can be lowered/increased


def extract_text_from_file(file_path):
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            return f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return ""


def scan_local_directory(scan_path):
    parsed_files = 0
    total_characters = 0
    skipped_large_files = 0

    print(f"\nScanning directory: {scan_path}\n")

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
                    total_characters += len(text)

                    print(f"Parsed: {file_path}")
                    print(f"Size: {file_size} bytes")
                    print(f"Characters: {len(text)}\n")

    print(f"Total Parsed Files: {parsed_files}")
    print(f"Total Characters Extracted: {total_characters}")
    print(f"Skipped Large Files: {skipped_large_files}")


def main():
    scan_path = input("Enter directory path to scan: ").strip()

    if not os.path.exists(scan_path):
        print("Error: Directory does not exist.")
        return

    if not os.path.isdir(scan_path):
        print("Error: Path is not a directory.")
        return

    scan_local_directory(scan_path)


if __name__ == "__main__":
    main()
