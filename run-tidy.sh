#!/usr/bin/env zsh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT/cmake-build-debug"
AUTO_FIX=0
FIX_ERRORS=0

usage() {
  cat <<'EOF'
Usage: run-tidy.sh [build_dir] [--autofix] [--fix-errors] [--check-only] [--help]

Options:
  --autofix      Enable automatic fixes via run-clang-tidy -fix
  --fix-errors   Also use -fix-errors (implies --autofix)
  --check-only   Force analysis-only mode without fixes
  --help         Show this help

Notes:
  By default, the script builds first so C++20 module artifacts exist.
  Set SKIP_BUILD=1 to skip the build step.
  Optionally set HELIOS_TIDY_C_COMPILER and HELIOS_TIDY_CXX_COMPILER.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --autofix)
      AUTO_FIX=1
      shift
      ;;
    --fix-errors)
      AUTO_FIX=1
      FIX_ERRORS=1
      shift
      ;;
    --check-only)
      AUTO_FIX=0
      FIX_ERRORS=0
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
      BUILD_DIR="$1"
      shift
      ;;
  esac
done

ensure_ninja_build_dir() {
  if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    if ! grep -q '^CMAKE_GENERATOR:INTERNAL=Ninja$' "$BUILD_DIR/CMakeCache.txt"; then
      echo "Build directory is not using Ninja, resetting $BUILD_DIR" >&2
      rm -rf "$BUILD_DIR"
    fi
  fi
}

if ! command -v run-clang-tidy >/dev/null 2>&1; then
  echo "run-clang-tidy not found. Please install llvm/clang-tools." >&2
  exit 1
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy not found. Please install llvm/clang-tools." >&2
  exit 1
fi

ensure_ninja_build_dir

if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
  echo "Generating compile_commands.json in $BUILD_DIR" >&2
  CMAKE_ARGS=(-S "$ROOT" -B "$BUILD_DIR" -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

  # Prefer explicit compiler overrides, otherwise use clang/clang++ from PATH if available.
  if [[ -n "${HELIOS_TIDY_C_COMPILER:-}" && -n "${HELIOS_TIDY_CXX_COMPILER:-}" ]]; then
    CMAKE_ARGS+=(
      "-DCMAKE_C_COMPILER=${HELIOS_TIDY_C_COMPILER}"
      "-DCMAKE_CXX_COMPILER=${HELIOS_TIDY_CXX_COMPILER}"
    )
  elif command -v clang >/dev/null 2>&1 && command -v clang++ >/dev/null 2>&1; then
    CMAKE_ARGS+=(
      -DCMAKE_C_COMPILER="$(command -v clang)"
      -DCMAKE_CXX_COMPILER="$(command -v clang++)"
    )
  fi
  cmake "${CMAKE_ARGS[@]}"
fi

if [[ "${SKIP_BUILD:-0}" != "1" ]]; then
  # Build generates module dependency artifacts required by clang-tidy for .ixx.
  cmake --build "$BUILD_DIR" --target helios_core -j "${CLANG_TIDY_BUILD_JOBS:-4}"
fi

# Analyze production code (.ixx modules + .cpp units)
PATTERN="$ROOT/src/.*\\.(ixx|cpp)$"
RUN_TIDY_CMD=(run-clang-tidy -p "$BUILD_DIR" -config-file="$ROOT/.clang-tidy")

if [[ "$AUTO_FIX" == "1" ]]; then
  RUN_TIDY_CMD+=(-fix)
fi

if [[ "$FIX_ERRORS" == "1" ]]; then
  RUN_TIDY_CMD+=(-fix-errors)
fi

RUN_TIDY_CMD+=("$PATTERN")
"${RUN_TIDY_CMD[@]}"

