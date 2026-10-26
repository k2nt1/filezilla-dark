# Manual Windows build workflow

The [Windows x64 workflow](../../.github/workflows/windows-x64.yml) builds the
source and runs upstream and palette tests on a GitHub-hosted Windows Server
2022 runner. It uses the repository's dependency manifest and build scripts with
MSYS2 UCRT64. Building on this runner does not establish runtime compatibility
with every Windows 10 or Windows 11 build.

The workflow runs only after a maintainer chooses **Run workflow** in GitHub
Actions. A push, pull request, or tag does not trigger it. Its token has
`contents: read`; checkout credentials are not persisted, and no repository
secrets are used. The last step uploads validation logs as a temporary Actions
artifact. It does not upload executable files or create a release.

Action versions were checked against their official release tags and pinned to
the corresponding commit:

| Action | Version | Commit |
| --- | --- | --- |
| [actions/checkout](https://github.com/actions/checkout/releases/tag/v7.0.1) | v7.0.1 | `3d3c42e5aac5ba805825da76410c181273ba90b1` |
| [msys2/setup-msys2](https://github.com/msys2/setup-msys2/releases/tag/v2.33.0) | v2.33.0 | `ec48f7c5447b3140e2b088413ae3a55687bccb6e` |
| [actions/upload-artifact](https://github.com/actions/upload-artifact/releases/tag/v7.0.2) | v7.0.2 | `cf430e030ddbb5b0abf93d22962f4752f3646cd9` |

The [MSYS2 action documentation](https://github.com/msys2/setup-msys2) describes
the `msystem`, `update`, and `install` inputs and the `msys2 {0}` shell.
[MSYS2 environment documentation](https://www.msys2.org/docs/environments/)
identifies UCRT64 as the x64 GCC environment using the Universal C Runtime.

The dependency source archives and action commits are pinned. MSYS2 package
repositories and hosted runner images continue to receive updates, so this is
not a bit-for-bit reproducible environment. The workflow has been prepared and
reviewed locally; it has not been dispatched or reported as a passing hosted CI
run.
