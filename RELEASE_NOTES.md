## Paw IDE 1.2.0

The IDE starts to run your code, grows a plugin catalog, and double-checks itself.
And the first official plugin exists: a full Russian translation of the
interface, plus the plugin runtime it needs.

### Added
- **Plugin runtime (phase 1)**: the IDE now scans `%APPDATA%\Paw\PawIDE\plugins`
  (and `<install>\plugins` for portable setups) for folders with a
  `plugin.json` manifest and activates the plugin types it understands —
  today, language packs (`type: "language-pack"`, a compiled Qt translation
  `.qm` installed before any window is created).
- The plugin catalog detects plugins installed on disk and marks them
  *active*, and gained an **Open plugins folder** shortcut plus a link to the
  official plugin repositories.
- **Official plugin: Language Pack — Русский** (`cartik-dev/pawide-langpack-ru`)
  translates every menu, dialog, panel, message and status-bar label of the IDE
  into Russian. Drop the plugin folder into the plugins directory and restart.

## Paw IDE 1.1.0
### Added
- **Plugin catalog** (`Extensions` side view, `Ctrl+Shift+X`): searchable offline
  catalog of 10 plugins with authors and versions, one-click install, installed
  state persisted in the settings database.
- **Run & Debug workflows** (`Run and Debug` side view, `Ctrl+Shift+D`): named
  run configurations (Run / Debug / Task) executed in the project folder as a
  child process; output streams into the bottom `Output` tab; add, delete and
  stop workflows; defaults are generated for CMake projects.
- **Sandbox verification** — the editor no longer only guesses:
  - patterns it is *certain* about (e.g. `if (x = 10)` — assignment used as a
    condition) are flagged immediately with a red wavy underline;
  - patterns it is *not sure* about (e.g. a declared variable that is never
    used) get an amber underline and stay silent;
  - if the user does not see or fix the suspicion for **15 seconds**, the editor
    copies the current buffer into an **isolated temporary directory where
    nothing is saved**, runs the configured compiler in syntax-check mode
    (`-fsyntax-only -Wall -Wextra`, g++ / clang++), and deletes the directory
    afterwards;
  - verdict: confirmed suspicions turn into red "confirmed by sandbox" errors,
    false alarms are cleared with a card in the corner; a card also reports the
    running and sure-error states.

### Changed
- Problems tab is now a real list with live entries instead of a placeholder.

## Paw IDE 1.1.0

The editor grows up: syntax highlighting, split view, search, terminal and Git arrive.

### Added
- Syntax highlighting for 30+ languages: keywords, strings, numbers, comments,
  multi-line block comments / Python doc strings, preprocessor directives
- Find & Replace in the editor (`Ctrl+F` / `Ctrl+H`) with match highlighting,
  live match counter, Replace / Replace All
- Split Editor (`Ctrl+\`) — a second pane with the same file; the find bar works in both panes
- Project-wide search (side panel): Match Case / Whole Word, results grouped by file,
  a click opens the file at the exact line
- Integrated terminal (bottom panel, ``Ctrl+` ``): PowerShell on Windows with CMD fallback,
  bash on other systems; starts lazily in the project folder
- Source Control side panel: branch, changed files, stage / unstage, commit,
  and `File > Clone Repository…`
- Automatic update checks against GitHub Releases (silent on startup,
  `Help > Check for Updates…` at any time)

## Paw IDE 1.0.0

First public release of the Paw IDE desktop application for **Windows 10 / 11 (x64)**.

### Included
- VS Code-style workspace: activity bar, side bar, editor tabs, bottom panel, status bar, menus
- Command Palette (`Ctrl+Shift+P`) and Go to File (`Ctrl+P`)
- Welcome screen with Open Folder, New Project and Recent Projects
- Open any folder as a project; several projects in separate windows
- New Project templates: Empty, C++, Python, Web, Rust, Lua
- File Explorer with hidden files shown by default; create, rename, delete, copy, paste, duplicate
- Code editor with line numbers, auto-indent, auto-closing brackets, bracket matching, word wrap
- Safe file handling: UTF-8 / UTF-8 BOM / UTF-16 / Latin-1, CRLF and LF preserved, atomic saves
- Settings stored in SQLite; unsaved changes are never dropped

### Not included yet
Plugins and marketplace.

### Installing
Download `PawIDE-Setup-x64.exe` and run it. The installer is **not code-signed**, so Windows
SmartScreen may show a warning ("More info" → "Run anyway"). Verify the download with
`PawIDE-Setup-x64.exe.sha256`:

```powershell
(Get-FileHash .\PawIDE-Setup-x64.exe -Algorithm SHA256).Hash.ToLower()
```
