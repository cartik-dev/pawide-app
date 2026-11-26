# Paw IDE

A desktop IDE written in C++20 with Qt 6. VS Code-style layout, black / white / violet identity.

Open source under the [MIT License](LICENSE). The Paw IDE name and the brand files in
`assets/branding` are not covered by that license (see `assets/branding/README.md`).

## Status

| Area | State |
|------|-------|
| Main window: activity bar, side bar, editor tabs, bottom panel, status bar, menus | done |
| Command Palette (`Ctrl+Shift+P`) and Go to File (`Ctrl+P`) | done |
| Welcome screen: Open Folder, New Project, Recent, Documentation | done |
| Open any folder as a project (system folder dialog) | done |
| New Project with real templates: Empty, C++, Python, Web, Rust, Lua | done |
| Recent projects (SQLite) – open / remove / clear, folders are never deleted | done |
| Several windows (`File > New Window`, open a folder in a new window) | done |
| File Explorer: nested folders, hidden files shown by default, create / rename / delete / copy / paste / duplicate / refresh / reveal | done |
| Editor: line numbers, tabs, auto-indent, auto-close brackets, bracket matching, word wrap, go to line, undo/redo | done |
| Safe file IO: UTF-8, UTF-8 BOM, UTF-16, Latin-1 fallback, CRLF/LF/CR preserved, atomic saves, binary files refused, async loading | done |
| Unsaved changes are never dropped on close / folder switch | done |
| Settings dialog (editor, explorer, about) stored in SQLite | partial |
| Syntax highlighting for 30+ languages (keywords, strings, numbers, comments, block-comment state, preprocessor) | done |
| Find & Replace in the editor (`Ctrl+F` / `Ctrl+H`), match highlighting, Replace All | done |
| Split editor (`Ctrl+\`), find bar works in both panes | done |
| Project-wide search (Search side panel, Match Case / Whole Word, click opens the file at the line) | done |
| Integrated terminal (bottom panel, PowerShell / CMD / bash, ``Ctrl+` ``) | done |
| Git integration: status, branch, stage / unstage, commit, `File > Clone Repository…` | done |
| Auto updater: checks GitHub Releases on startup and via `Help > Check for Updates…` | done |
| Plugins, SDK, marketplace – Phase 4 | not started |
| Windows installer, release pipeline – Phase 6 | done – v1.0.0 released, CI builds `PawIDE-Setup-x64.exe` and publishes it to [GitHub Releases](https://github.com/cartik-dev/pawide-app/releases) |

Side-bar pages for Run and Extensions are explicit placeholders.

## Build

Requirements: CMake ≥ 3.21, a C++20 compiler, Qt ≥ 6.4 (Widgets, Gui, Sql, Svg, Concurrent, Network, Test).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
./build/PawIDE [folder | file ...]
```

`PAWIDE_DB=<path>` relocates the settings database (default: the per-user app data folder).

## Tests

`ctest` runs six QtTest executables (file IO / encodings / line endings, project templates,
database, file operations, syntax highlighting, and an offscreen GUI test that opens folders, lists
hidden and nested files, edits and saves files, and checks the unsaved-changes protection).

## Layout

```
assets/branding   official brand files copied verbatim from the website (see its README)
resources/        Qt resource bundle
src/core          file IO, language detection, app info
src/database      SQLite storage (settings, recent projects)
src/settings      typed settings
src/project       project templates
src/explorer      file explorer + file operations
src/editor        code editor, tabbed panes, split view, find bar, syntax highlighting
src/search        project-wide search panel
src/terminal      integrated terminal (PowerShell / CMD / bash)
src/scm           Git panel (status, stage, commit, clone)
src/network       auto updater (GitHub Releases)
src/ui            main window, welcome screen, dialogs, theme
tests/            unit and GUI tests
tools/            developer screenshot tool
```
