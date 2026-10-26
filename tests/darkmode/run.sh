#!/usr/bin/env bash
# Build and run the helper against this checkout, using an existing UCRT64 build.
set -euo pipefail
export PATH="/ucrt64/bin:/usr/bin:$PATH"

test_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "$test_dir/../.." && pwd)
build_root=${FZ_BUILD_ROOT:-"$repo_root/../filezilla-dark-build"}
prefix=${FZ_PREFIX:-"$build_root/prefix"}
app_build="$build_root/build/filezilla"
output="$build_root/test-results/darkmode"
compile_only=0

usage() {
  cat <<'USAGE'
Usage: bash tests/darkmode/run.sh [options]

  --prefix DIR       Installed custom dependencies, including bin/wx-config.
  --app-build DIR    Configured FileZilla build containing config/config.h.
  --output DIR       Test executable and results (outside source by default).
  --compile-only     Compile without starting the hidden-window test program.
  --help             Show this message.

Defaults follow FZ_BUILD_ROOT and FZ_PREFIX from scripts/build-windows.sh.
Run in an MSYS2 UCRT64 shell after building the application dependencies.
USAGE
}

while (($#)); do
  case "$1" in
    --prefix|--app-build|--output)
      if (($# < 2)) || [[ -z "$2" ]]; then
        printf 'Missing directory after %s\n' "$1" >&2
        exit 2
      fi
      case "$1" in
        --prefix) prefix=$2 ;;
        --app-build) app_build=$2 ;;
        --output) output=$2 ;;
      esac
      shift 2
      ;;
    --compile-only) compile_only=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) printf 'Unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ ${MSYSTEM:-} != UCRT64 ]]; then
  printf 'Use an MSYS2 UCRT64 shell (MSYSTEM=UCRT64).\n' >&2
  exit 2
fi
to_msys_path() {
  case "$1" in
    [A-Za-z]:*|\\\\*) cygpath -u "$1" ;;
    *) printf '%s\n' "$1" ;;
  esac
}
prefix=$(to_msys_path "$prefix")
app_build=$(to_msys_path "$app_build")
output=$(to_msys_path "$output")
prefix=$(cd -- "$prefix" && pwd)
app_build=$(cd -- "$app_build" && pwd)
export PATH="$prefix/bin:/ucrt64/bin:/usr/bin:$PATH"
wx_config=${WX_CONFIG:-"$prefix/bin/wx-config"}
cxx=${CXX:-g++}
windres=${WINDRES:-windres}
for tool in "$cxx" "$windres" timeout; do
  command -v "$tool" >/dev/null || { printf 'Required tool is missing: %s\n' "$tool" >&2; exit 2; }
done
[[ -x "$wx_config" ]] || { printf 'wx-config not found: %s\n' "$wx_config" >&2; exit 2; }
[[ -f "$app_build/config/config.h" ]] || {
  printf 'Missing %s/config/config.h; run scripts/build-windows.sh app first.\n' "$app_build" >&2
  exit 2
}
[[ $("$cxx" -dumpmachine) == x86_64-w64-mingw32 ]] || {
  printf 'This test manifest requires the Windows x64 MinGW compiler.\n' >&2
  exit 2
}
mkdir -p -- "$output"
output=$(cd -- "$output" && pwd)

# wx-config is the trusted script from the dependency build. Its output is
# shell-quoted compiler arguments; arrays also preserve paths containing spaces.
eval "wx_cxxflags=($("$wx_config" --cxxflags))"
eval "wx_libs=($("$wx_config" --libs std,aui))"
{
  "$cxx" --version
  printf '\nwxWidgets '
  "$wx_config" --version
} > "$output/toolchain.txt"

"$windres" -I"$test_dir" "$test_dir/test-dark-palette.rc" -O coff -o "$output/test-dark-palette-res.o"
"$cxx" -std=c++17 -DHAVE_CONFIG_H \
  -I"$app_build/config" -I"$prefix/include" -I"$repo_root/src/interface" \
  -ffile-prefix-map="$repo_root=." -fdebug-prefix-map="$repo_root=." \
  "${wx_cxxflags[@]}" \
  "$test_dir/test-dark-palette.cpp" "$repo_root/src/interface/dark_theme.cpp" \
  "$output/test-dark-palette-res.o" "${wx_libs[@]}" -ladvapi32 \
  -o "$output/test-dark-palette.exe"

if ((compile_only)); then
  printf 'Compiled palette test; execution was skipped.\n'
else
  # Only this test process changes appearance. Windows preferences stay intact.
  timeout 120s "$output/test-dark-palette.exe" 2>&1 | tee "$output/results.txt"
fi
