# Windows build

Build in an **MSYS2 UCRT64** shell on Windows. The application and wxWidgets must be rebuilt together; the fork uses wxWidgets 3.3.4 APIs. The commands below install only into a private build prefix and do not replace an installed FileZilla client.

## Dependencies

Update MSYS2 using its normal update procedure, then install the package list from the repository root:

```bash
mapfile -t packages < <(grep -vE '^[[:space:]]*(#|$)' scripts/msys2-packages.txt)
pacman -S --needed "${packages[@]}"
```

The i686 compiler is included for the 32-bit Explorer shell extension; the application itself is x64. The build creates architecture-prefixed `windres` wrappers in its own tools directory instead of changing MSYS2's installed tool files.

Pinned external sources are listed in [DEPENDENCIES.json](DEPENDENCIES.json):

| Component | Version |
|---|---|
| wxWidgets | 3.3.4 |
| libfilezilla | 0.58.0 |
| fzssh | 1.5.0 |

`scripts/fetch-dependencies.py` validates each archive against its pinned SHA-256/SHA-512 before use. FileZilla's source download CDN may require expiring URLs; the two smaller library archives can instead be obtained from Arch Linux's source mirror and must match the publisher's published hashes exactly. Source archives, extraction directories, receipts, logs, and outputs stay outside the repository by default.

MSYS2 package versions are rolling. The development build used GCC/G++ 16.2.0; the source archive versions and hashes are pinned, but byte-for-byte reproduction also requires matching the toolchain/package versions.

## Build and test

From the repository root:

```bash
bash scripts/build-windows.sh all
bash scripts/build-windows.sh check
```

`all` builds and installs libfilezilla, fzssh, wxWidgets, and FileZilla. `check` then runs the three projects' existing regression suites. Translation catalogs, FTP/FTPS/SFTP, and the 32/64-bit shell-extension build remain enabled. wxWidgets' unused webview component is omitted.

The default layout is:

```text
repository/
../filezilla-dark-build/
  downloads/                 verified source archives and receipt
  sources/                   extracted external sources
  build/libfilezilla-0.58.0/
  build/fzssh-1.5.0/
  build/wx-3.3.4/
  build/filezilla/
  prefix/                    private headers, libraries, application/resources
  logs/
  tools/bin/                 local cross-resource-compiler wrappers
```

The installed application is `prefix/bin/filezilla.exe`. The build defines `FZ_DARK_PORTABLE=1`, so this application's settings are adjacent to its executable. Do not use an installed application's DLLs to fill missing dependencies: assemble a complete bundle from the same prefix and matching MSYS2 runtimes.

Environment variables:

| Variable | Default / purpose |
|---|---|
| `FZ_BUILD_ROOT` | Repository's sibling `filezilla-dark-build`; accepts a Windows or MSYS path |
| `FZ_PREFIX` | `$FZ_BUILD_ROOT/prefix`; private installation prefix |
| `FZ_JOBS` | `4`; parallel compile jobs |
| `FZ_REGENERATE` | Set to `1` to run `autoreconf -i` in the FileZilla source |

Individual stages are available: `dependencies`, `wx`, `app`, and `check`. For example:

```bash
export FZ_BUILD_ROOT="$(pwd)/.build"
export FZ_PREFIX="$FZ_BUILD_ROOT/prefix"
export FZ_JOBS=4
bash scripts/build-windows.sh all
bash scripts/build-windows.sh check
```

Use paths without spaces for this Autotools/Windows toolchain. `FZ_BUILD_ROOT` and `FZ_PREFIX` should be dedicated writable build directories. The script rejects a build or prefix directory that would overwrite the source checkout itself.

## Additional appearance tests

See [tests/darkmode/README.md](tests/darkmode/README.md) for the wxWidgets/application palette harness. Those tests use the already built prefix and application configuration. Existing engine tests and appearance tests have different scopes; report their results separately.

## Public binary releases

The scripts add compiler file/debug/macro prefix maps to reduce embedded local source paths. For a public binary, build in a neutral workspace or hosted CI, inspect the finished artifacts for local paths and personal data, and package a fresh empty profile rather than a development profile. Keep private fixture keys, credentials, transfer data, and unrelated screenshots out of release assets.

The maps also cover the native MSYS2 installation prefix, including standard-library assertion locations. After configure, `scripts/normalize-buildinfo.py` replaces that machine-specific prefix only in the generated `USED_CXXFLAGS` display string used by About/build information. The real compiler options stay intact, and assertions, application source, and executable bytes are not patched. Raw build logs and generated configuration files can still contain local paths and should remain private.

Include the exact runtime libraries and their license texts, a version/hash inventory, and access to the corresponding source/build material required by their licenses. The repository's FileZilla source alone does not supply source for all separately linked runtime libraries. Preserve fzssh's documentation and About/legal attribution. See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
