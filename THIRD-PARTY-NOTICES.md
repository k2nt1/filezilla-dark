# Upstream and third-party notices

FileZilla Client is copyright (C) 2004-2026 Tim Kosse. Its source is provided under GNU GPL version 3 or later, as stated in the original `README`. The upstream `COPYING`, author notices, and bundled resource/source notices are preserved. `LICENSE` duplicates the original GPL version 3 text without modification.

This project does not grant new licenses for upstream or third-party material. The following describes the core components; a binary bundle must also carry the notices for every additional runtime it actually includes.

| Component | Applicable notice / license |
|---|---|
| FileZilla Client source and engine | GPL version 3 or later; [LICENSE](LICENSE), original `README`/`COPYING` |
| libfilezilla 0.58.0 | GPL version 2 or later; [upstream notice](LICENSES/libfilezilla-README.txt), [GPL v2 text](LICENSES/GPL-2.0.txt) |
| fzssh 1.5.0 | AGPL version 3 or later **with additional attribution terms**; [unaltered notice](LICENSES/fzssh-README.txt), [AGPL v3 text](LICENSES/AGPL-3.0.txt) |
| wxWidgets 3.3.4 | wxWindows Library Licence 3.1 or later, including its exception; [licence](LICENSES/wxWidgets-LICENCE.txt), [underlying Library GPL text](LICENSES/wxWidgets-LGPL.txt) |
| Bundled pugixml | MIT; [notice copied from the bundled header](LICENSES/pugixml-MIT.txt); original source notices remain in `src/pugixml/` |

GPL v3 section 13 permits a GPL-covered work to be combined with an AGPL v3 work. The GPL terms continue to apply to the GPL-covered part, and the AGPL section 13 network-interaction requirements apply to the combination as described there. The fzssh component is therefore not described as GPL-only or relicensed by this fork.

## Required fzssh attribution

**This software incorporates components from fzssh, copyright (c) 2025-2026 Tim Kosse and Business Follows Srl.**

The full upstream fzssh README is included unchanged. It requires preserving copyright/authorship notices, the above attribution in accompanying documentation when conveying binaries, and appropriate copyright attribution in interactive legal/About information. FileZilla's existing About/legal information includes the fzssh authorship notice and must remain present.

## Corresponding source and binary bundles

The source repository contains FileZilla and its upstream bundled material, not vendored copies of every external dependency. `DEPENDENCIES.json` and the fetch/build scripts identify the pinned core dependency sources. Keep the verified source archives and build scripts for a binary release available to recipients as required by the applicable licenses.

MSYS2-built libraries can include package-specific patches and build recipes. A runtime DLL inventory or hash list is not the same as its corresponding source. Preserve the exact package versions, package recipes/patches, upstream source access, and all applicable licenses/notices when preparing a public binary bundle. Runtime libraries may have different licenses or runtime exceptions; use their own notices rather than assigning one project-wide license label to every file.

Primary references: the included license texts and upstream READMEs, [FileZilla](https://filezilla-project.org/), [libfilezilla](https://lib.filezilla-project.org/), [fzssh](https://fzssh.filezilla-project.org/), and [wxWidgets licensing](https://www.wxwidgets.org/about/licence/).
