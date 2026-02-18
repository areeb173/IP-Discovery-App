import os
from git import Repo, GitCommandError

REPO_URL = "https://github.com/AbdullahMoghal/t-insight-hack2025"
CLONE_DIR = "temp_repo"
SUPPORTED_EXTENSIONS = {
    ".py", ".js", ".ts", ".java", ".cs",
    ".cpp", ".c", ".md", ".txt", ".json", ".yaml", ".yml"
}

def clone_or_update_repository(repo_url, clone_dir):
    if os.path.exists(clone_dir):
        print("Repository already exists. Pulling latest changes...\n")
        try:
            repo = Repo(clone_dir)
            repo.remotes.origin.pull()
            print("Repository updated.\n")
        except GitCommandError as e:
            print(f"Error pulling repository: {e}")
    else:
        print(f"Cloning repository: {repo_url}")
        Repo.clone_from(repo_url, clone_dir)
        print("Clone completed.\n")


def extract_text_from_file(file_path):
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            return f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return ""


def scan_repository(repo_path):
    parsed_files = 0
    total_characters = 0

    for root, dirs, files in os.walk(repo_path):
        for file in files:
            ext = os.path.splitext(file)[1].lower()

            if ext in SUPPORTED_EXTENSIONS:
                file_path = os.path.join(root, file)
                text = extract_text_from_file(file_path)

                if text.strip():
                    parsed_files += 1
                    total_characters += len(text)

                    print(f"Parsed: {file_path}")
                    print(f"Characters: {len(text)}\n")

    print(f"Total Parsed Files: {parsed_files}")
    print(f"Total Characters Extracted: {total_characters}")


def main():
    clone_or_update_repository(REPO_URL, CLONE_DIR)
    scan_repository(CLONE_DIR)


if __name__ == "__main__":
    main()
