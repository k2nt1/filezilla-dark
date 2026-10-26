#!/usr/bin/env bash
set -euo pipefail

[[ "${MSYSTEM:-}" == UCRT64 ]] || { echo 'Run this script in an MSYS2 UCRT64 shell.' >&2; exit 2; }
fz_repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fz_build="$(cygpath -u "${FZ_BUILD_ROOT:-$fz_repo/../filezilla-dark-build}")"
fz_build="$(realpath -m "$fz_build")"
fz_prefix="$(realpath -m "$(cygpath -u "${FZ_PREFIX:-$fz_build/prefix}")")"
fz_jobs="${FZ_JOBS:-4}"
fz_stage="${1:-all}"
case "$fz_stage" in all|dependencies|wx|app|check) ;; *) echo 'Stage must be all, dependencies, wx, app, or check.' >&2; exit 2;; esac
[[ "$fz_jobs" =~ ^[1-9][0-9]*$ ]] || { echo 'FZ_JOBS must be positive.' >&2; exit 2; }
for fz_path in "$fz_repo" "$fz_build" "$fz_prefix"; do
  [[ "$fz_path" != *[[:space:]]* ]] || { echo 'Use paths without spaces for this Windows Autotools build.' >&2; exit 2; }
done
[[ "$fz_build" != "$fz_repo" && "$fz_prefix" != "$fz_repo" ]] || { echo 'Do not install over the source checkout.' >&2; exit 2; }
mkdir -p "$fz_build"/{downloads,sources,build,logs,tools/bin} "$fz_prefix"
export PATH="$fz_build/tools/bin:$fz_prefix/bin:$fz_prefix/lib:/ucrt64/bin:/usr/bin:/mingw32/bin:$PATH"
export PKG_CONFIG_PATH="$fz_prefix/lib/pkgconfig:/ucrt64/lib/pkgconfig"
export CC=gcc CXX=g++

# Cover native and MSYS spellings of source/build paths in __FILE__ and debug
# information. A neutral public-release workspace is still recommended because
# configure-time data-directory literals are separate from compiler prefix maps.
fz_map_flags=""
for fz_pair in "$fz_repo=/src/filezilla-dark" "$fz_build=/build/filezilla-dark"; do
  fz_from="${fz_pair%%=*}"
  fz_to="${fz_pair#*=}"
  for fz_spelling in "$fz_from" "$(cygpath -m "$fz_from")"; do
    fz_map_flags+=" -ffile-prefix-map=$fz_spelling=$fz_to -fdebug-prefix-map=$fz_spelling=$fz_to -fmacro-prefix-map=$fz_spelling=$fz_to"
  done
done
export CFLAGS="-O2 -DNDEBUG$fz_map_flags"
export CXXFLAGS="$CFLAGS"

fz_fetch() {
  python "$fz_repo/scripts/fetch-dependencies.py" --cache "$fz_build/downloads"
  for fz_archive in libfilezilla-0.58.0.tar.xz fzssh-1.5.0.tar.xz wxWidgets-3.3.4.tar.bz2; do
    case "$fz_archive" in
      libfilezilla*) fz_directory=libfilezilla-0.58.0;;
      fzssh*) fz_directory=fzssh-1.5.0;;
      wxWidgets*) fz_directory=wxWidgets-3.3.4;;
    esac
    [[ -d "$fz_build/sources/$fz_directory" ]] || tar -xf "$fz_build/downloads/$fz_archive" -C "$fz_build/sources"
  done
}

fz_dependencies() {
  fz_fetch
  mkdir -p "$fz_build/build/libfilezilla-0.58.0"
  (
    cd "$fz_build/build/libfilezilla-0.58.0"
    bash "$fz_build/sources/libfilezilla-0.58.0/configure" --prefix="$fz_prefix" \
      --host=x86_64-w64-mingw32 --build=x86_64-w64-mingw32 --enable-shared --disable-static
    make -j"$fz_jobs"
    make install
  )
  fz_meson_args=(setup "$fz_build/build/fzssh-1.5.0" "$fz_build/sources/fzssh-1.5.0"
    --prefix="$(cygpath -m "$fz_prefix")" --libdir=lib --buildtype=release --default-library=shared)
  [[ ! -f "$fz_build/build/fzssh-1.5.0/build.ninja" ]] || fz_meson_args+=(--reconfigure)
  meson "${fz_meson_args[@]}"
  meson compile -C "$fz_build/build/fzssh-1.5.0" -j"$fz_jobs"
  meson install -C "$fz_build/build/fzssh-1.5.0"
  pkg-config --modversion libfilezilla libfzssh-client
}

fz_wx() {
  fz_fetch
  mkdir -p "$fz_build/build/wx-3.3.4"
  (
    cd "$fz_build/build/wx-3.3.4"
    bash "$fz_build/sources/wxWidgets-3.3.4/configure" --prefix="$fz_prefix" \
      --build=x86_64-w64-mingw32 --enable-shared --enable-printfposparam \
      --disable-debug --enable-optimise --with-msw --disable-webview \
      --with-libjpeg=builtin --with-libpng=builtin --with-libtiff=builtin \
      --with-regex=builtin --with-expat=builtin --with-zlib=builtin
    make -j"$fz_jobs"
    make install
  )
  "$fz_prefix/bin/wx-config" --version
}

fz_app() {
  if [[ "${FZ_REGENERATE:-0}" == 1 || ! -f "$fz_repo/configure" ]]; then
    (cd "$fz_repo" && autoreconf -i)
  fi
  # Only create wrappers in this build's own directory, not the MSYS2 install.
  for fz_arch in x86_64-w64-mingw32 i686-w64-mingw32; do
    [[ "$fz_arch" != i686* ]] && fz_native=/ucrt64/bin/windres.exe || fz_native=/mingw32/bin/windres.exe
    [[ -f "$fz_native" ]] || { echo "Missing resource compiler: $fz_native" >&2; exit 2; }
    printf '#!/usr/bin/env bash\nexec "%s" "$@"\n' "$fz_native" > "$fz_build/tools/bin/$fz_arch-windres"
    chmod +x "$fz_build/tools/bin/$fz_arch-windres"
  done
  mkdir -p "$fz_build/build/filezilla"
  (
    cd "$fz_build/build/filezilla"
    # Let each sub-configure select its host-prefixed compiler; exporting
    # CC=gcc here would make the i686 extension use the x64 compiler.
    unset CC CXX
    CFLAGS="$CFLAGS -DFZ_DARK_PORTABLE=1" CXXFLAGS="$CXXFLAGS -DFZ_DARK_PORTABLE=1" \
      bash "$fz_repo/configure" --prefix="$fz_prefix" \
      --build=x86_64-w64-mingw32 --host=x86_64-w64-mingw32 \
      --with-wx-config="$fz_prefix/bin/wx-config" --with-pugixml=builtin
    make -j"$fz_jobs"
    make install
  )
}

fz_check() {
  make -C "$fz_build/build/libfilezilla-0.58.0" check -j"$fz_jobs"
  meson test -C "$fz_build/build/fzssh-1.5.0" --print-errorlogs
  make -C "$fz_build/build/filezilla" check
}

exec > >(tee "$fz_build/logs/$fz_stage.log") 2>&1
case "$fz_stage" in
  all) fz_dependencies; fz_wx; fz_app;;
  dependencies) fz_dependencies;;
  wx) fz_wx;;
  app) fz_app;;
  check) fz_check;;
esac
