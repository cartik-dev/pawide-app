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
| Syntax highlighting (Tree-sitter) – Phase 2 | not started |
| Split editor, find & replace, project search – Phase 2 | not started |
| Terminal, Git integration, "Clone Repository" – Phase 3 | not started |
| Plugins, SDK, marketplace – Phase 4 | not started |
| Asset manager, auto updater – Phase 5 | not started |
| Windows installer, release pipeline – Phase 6 | not started |

Side-bar pages for Search, Source Control, Run and Extensions are explicit placeholders.

## Build

Requirements: CMake ≥ 3.21, a C++20 compiler, Qt ≥ 6.4 (Widgets, Gui, Sql, Svg, Concurrent, Test).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
./build/PawIDE [folder | file ...]
```

`PAWIDE_DB=<path>` relocates the settings database (default: the per-user app data folder).

## Tests

`ctest` runs five QtTest executables (file IO / encodings / line endings, project templates,
database, file operations, and an offscreen GUI test that opens folders, lists hidden and nested
files, edits and saves files, and checks the unsaved-changes protection).

## Layout

```
assets/branding   official brand files copied verbatim from the website (see its README)
resources/        Qt resource bundle
src/core          file IO, language detection, app info
src/database      SQLite storage (settings, recent projects)
src/settings      typed settings
src/project       project templates
src/explorer      file explorer + file operations
src/editor        code editor + tabbed editor area
src/ui            main window, welcome screen, dialogs, theme
tests/            unit and GUI tests
tools/            developer screenshot tool
```
