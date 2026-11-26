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
Syntax highlighting, split editor, find and replace, project search, integrated terminal,
Git integration, plugins and marketplace, automatic updates.

### Installing
Download `PawIDE-Setup-x64.exe` and run it. The installer is **not code-signed**, so Windows
SmartScreen may show a warning ("More info" → "Run anyway"). Verify the download with
`PawIDE-Setup-x64.exe.sha256`:

```powershell
(Get-FileHash .\PawIDE-Setup-x64.exe -Algorithm SHA256).Hash.ToLower()
```
