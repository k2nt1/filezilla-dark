# Dark palette integration test

Run from an **MSYS2 UCRT64** shell after building the dependencies and configuring
FileZilla as described in [BUILD.md](../../BUILD.md):

```bash
bash tests/darkmode/run.sh
```

The defaults follow `FZ_BUILD_ROOT` and `FZ_PREFIX` used by
`scripts/build-windows.sh`. Existing builds can be selected explicitly:

```bash
bash tests/darkmode/run.sh \
  --prefix /path/to/build/prefix \
  --app-build /path/to/build/build/filezilla \
  --output /path/to/build/test-results/darkmode
```

`--compile-only` compiles the program without running it. `CXX`, `WINDRES`, and
`WX_CONFIG` can select trusted compiler tools. The script uses an existing
dependency installation and does not download or install software.

The test compiles the palette helper directly from this checkout and links it
against the custom wxWidgets build. A common-controls version 6 manifest matches
the application's themed controls. Test windows remain hidden. App appearance
changes affect only the test process; the test reads the Windows app-color
preference to check Follow System and never writes it.

On a normal Windows desktop, 25 checks cover:

- Dark, Light, and Follow System appearance within the test process.
- Removal and reapplication of helper-owned default colors.
- Preservation of explicit site colors, warning colors, and later caller changes.
- Child-color inheritance, repeated shutdown, and queued-callback invalidation.
- At least 4.5:1 text contrast for dark log categories, comparison rows, and
  progress bars, plus the light progress fill.

Some dark-palette checks are skipped when Windows High Contrast is already
active. The test does not toggle High Contrast. A missing or unreadable app-color
registry preference also skips its comparison. `FAILURES=0` and exit status zero
indicate success; a compiler failure or any failed check returns nonzero.

Results are written to `results.txt` and toolchain details to `toolchain.txt` in
the selected output directory. These checks do not establish complete visual
coverage, runtime transfer correctness, or Windows-version compatibility. Manual
checks should still cover menus, dialogs, keyboard focus, disabled controls,
selection, scrolling, scaling, and transfers.

The palette test passed locally against this checkout on Windows 10 x64 with
wxWidgets 3.3.4: `CHECKS=25 FAILURES=0`. The GitHub Actions workflow is a prepared
manual build recipe; no hosted CI result is implied by this local result.
