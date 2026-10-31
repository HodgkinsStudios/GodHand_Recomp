# Local God Hand ISO and future GitHub Actions runner

**Current phase:** Planning/documentation only. No local ISO integration workflow or self-hosted runner installation has been implemented or registered by this initial checkpoint.

## Source versus game data

The source code lives in GitHub. The retail game disc image, extracted game content, game executable and locally generated game code should live on the user's machine **outside the Git checkout**. This separation helps prevent accidental redistribution and reduces the scope of CI uploads.

Example private layout (choose your own path):

```text
~/PS2GameData/GodHand/
├── GodHand.iso
├── extracted/
├── generated/
└── logs/
```

Do not store or hardcode a personal username, absolute home directory, ISO path, game asset or checksum that identifies a private file in committed configuration. For a future runner use a local, access-controlled configuration file or dedicated runner environment, never a committed ISO and never an Actions artifact containing game data.

## What will and will not work

- **Standard `ubuntu-latest`/Windows GitHub-hosted Actions** can compile the shared PS2Recomp source and run toolchain tests without the ISO, but cannot read an ISO stored only on your PC.
- **A self-hosted runner** on your own machine can read local files accessible to the runner account, including the ISO if explicitly granted permission. GitHub schedules the job; it does not magically upload or mount files from the PC into hosted runners.
- A workflow job runs command-line tools on the runner. ChatGPT can help change repository source and reason over available logs, but does **not** have direct filesystem access to your local ISO through GitHub's repository API.

## Security policy for the later runner setup

1. Prefer a **private repository** while initially developing; do not assume private means safe from malicious code.
2. Use a **dedicated unprivileged runner user**, preferably in a separate host/VM/container, with access to only what it needs. A dedicated VM/host is the strongest separation.
3. Make ISO locations **read-only** to the runner wherever practical. Do not run the runner as root, or give it unnecessary secrets and system access.
4. Prevent unreviewed pull requests and third-party workflow changes from executing on the local runner. Favor manually dispatched, reviewed tasks and restricted runner labels/groups.
5. Keep any workflow-generated output that contains original code/assets in a protected **local-only directory**. Avoid uploading complete recompiled game source, ISO contents, executables, dumps, media or sensitive trace snippets to GitHub Actions logs and artifacts.
6. Check workflow permissions and third-party actions carefully. Logs and artifacts are remote data, even when the job executes on the local PC.
7. Review `git status`, `git diff --cached --stat` and `git ls-files` for game-data paths before publishing.

## Current GitHub Actions configuration

The uploaded upstream `.github/workflows/build.yml` is present unchanged. It runs general toolchain builds/tests on GitHub-hosted Linux and Windows runners on push and pull request. It does **not** use a local ISO; a separate, explicitly approved workflow will be needed for local game work. Because the existing workflow triggers on `push`, it may run after the first GitHub push.

## Future milestone: restricted on-premises job

Later we can implement a manually triggered `runs-on: [self-hosted, linux, x64, godhand-local]` job and restrict it to a dedicated runner. Actual labels and paths will be chosen only after the user has configured the runner. The workflow should default to read-only source permissions, avoid downloading untrusted pull-request code, and initially only verify the ISO exists and report a non-sensitive result. No new workflow of this type is included yet.
