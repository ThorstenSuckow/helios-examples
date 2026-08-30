#!/usr/bin/env zsh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
TARGET="$ROOT/src"
CHECK_ONLY=0

usage() {
  cat <<'EOF'
Usage: run-format.sh [path] [--check-only] [--help]

Options:
  --check-only   Check formatting only (no file changes)
  --help         Show this help

Notes:
  - Uses clang-format with style from .clang-format in this module.
  - Default path is ./src
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --check-only)
      CHECK_ONLY=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    --*)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
    *)
      TARGET="$1"
      shift
      ;;
  esac
done

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found. Please install clang-format/llvm." >&2
  exit 1
fi

if [[ ! -f "$ROOT/.clang-format" ]]; then
  echo "Missing formatter config: $ROOT/.clang-format" >&2
  exit 1
fi

if [[ ! -e "$TARGET" ]]; then
  echo "Path does not exist: $TARGET" >&2
  exit 1
fi

# Include C++ source, headers, and module interface units.
EXTENSIONS=("*.cpp" "*.cxx" "*.cc" "*.h" "*.hpp" "*.ixx")
FILES=()

is_supported_file() {
  local f="$1"
  for pattern in "${EXTENSIONS[@]}"; do
    if [[ "$f" == ${~pattern} ]]; then
      return 0
    fi
  done
  return 1
}

if [[ -f "$TARGET" ]]; then
  if is_supported_file "$TARGET"; then
    FILES+=("$TARGET")
  else
    echo "File extension is not supported: $TARGET" >&2
    exit 1
  fi
else
  while IFS= read -r -d '' file; do
    FILES+=("$file")
  done < <(find "$TARGET" -type f \( -name '*.cpp' -o -name '*.cxx' -o -name '*.cc' -o -name '*.h' -o -name '*.hpp' -o -name '*.ixx' \) -print0)
fi

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No matching files found under: $TARGET"
  exit 0
fi

if [[ "$CHECK_ONLY" == "1" ]]; then
  echo "Checking formatting for ${#FILES[@]} file(s)..."
  for file in "${FILES[@]}"; do
    clang-format --style=file --dry-run --Werror "$file"
  done
  echo "Formatting check passed."
else
  echo "Formatting ${#FILES[@]} file(s)..."
  for file in "${FILES[@]}"; do
    clang-format -i --style=file "$file"
  done
  echo "Formatting complete."
fi

