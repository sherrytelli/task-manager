# Task Manager - Project Understanding

## Project Overview
A Qt6 C++17 desktop Task Manager application for Linux that displays live system process information in a GUI table.

## Project Structure
```
task-manager/
├── CMakeLists.txt          # CMake build config (Qt6 Core, Widgets, Gui)
├── Makefile                # Convenience targets: clean, build (Ninja), run
├── .gitignore              # Ignores build/ and .clangd
├── skills-lock.json        # Skill hashes (cpp-coding-standards, qt6-desktop-ux)
└── src/
    ├── main.cpp            # Entry point
    ├── mainwindow.h/cpp    # Main window with toolbar + page switching
    ├── processeswidget.h/cpp   # Process table + /proc data reader
```

## Architecture

### main.cpp
- Creates `QApplication` and `MainWindow`, shows the window, runs the event loop.

### MainWindow (`src/mainwindow.h`, `src/mainwindow.cpp`)
- **Window**: 600x700, titled "Task Manager".
- **Layout**: Central `QWidget` with a `QVBoxLayout` containing a `QStackedWidget`.
- **Toolbar**: Top toolbar (non-movable, non-floatable, text-only buttons) with two `QAction`s:
  - **Metrics** - shows `metricsScreen` (currently a black `QLabel` placeholder)
  - **Processes** - shows `processesScreen` (a `ProcessWidget`)
- **Navigation**: Toolbar actions toggle visibility and enable/disable each other. The Metrics screen is shown by default.

#### Member Variables
| Variable | Type | Purpose |
|---|---|---|
| `processesScreen` | `ProcessWidget*` | Processes tab content |
| `metricsScreen` | `QLabel*` | Metrics tab placeholder |
| `stackedWidget` | `QStackedWidget*` | Page container |
| `metricsAction` | `QAction*` | Toolbar action for Metrics |
| `processesAction` | `QAction*` | Toolbar action for Processes |

#### Methods
- `init()` - Sets up central widget, stacked widget, toolbar, actions, and connections
- `showMetricsScreen()` - Sets stacked widget index to 0, enables Metrics action, disables Processes
- `showProcessesScreen()` - Sets stacked widget index to 1, enables Processes action, disables Metrics

### ProcessWidget (`src/processeswidget.h`, `src/processeswidget.cpp`)
- **Parent class**: `QWidget`
- **Table**: `QTableWidget` with 3 columns: **PID**, **USER**, **COMMAND**
  - Non-editable (`NoEditTriggers`)
  - Row selection (`SelectRows`)
  - Vertical header hidden
- **Search**: `QLineEdit` at the bottom, filters rows in real-time (case-insensitive) by command name
- **Live refresh**: `QTimer` calls `updateProcessesList()` every 2 seconds
- **Data source**: Reads `/proc/[pid]/status` for each numeric directory in `/proc/`

#### Member Variables
| Variable | Type | Purpose |
|---|---|---|
| `tableWidget` | `QTableWidget*` | Process data table |
| `procDir` | `QDir*` | `/proc/` directory reader (filters Dirs, no dots) |
| `searchLineEdit` | `QLineEdit*` | Search input |
| `lastSearchText` | `QString` | Stores filter text to reapply after each refresh |

#### Methods
- `updateProcessesList()` - Clears table, iterates `/proc/` directories, reads each `status` file, extracts Name and Uid, populates table rows
- `filterProcesses(const QString &filterText)` - Hides/shows rows based on whether the command column contains the search text (case-insensitive). Stores text in `lastSearchText` for reapplication.

## Data Flow
1. User clicks "Processes" in toolbar
2. `ProcessWidget` constructor runs: creates table, search bar, `/proc` QDir, starts 2-second timer
3. `updateProcessesList()` is called immediately and then every 2 seconds
4. For each PID directory in `/proc/`:
   - Open `/proc/[pid]/status`
   - First line → `Name:` → extract process name via `line.slice(6)`
   - Skip 8 lines → `Uid:` → extract RUID via `line.slice(5, 4)`
   - Insert row: PID, UID, Name
5. After repopulation, `filterProcesses()` re-applies the current search filter

## Known Issues

### 1. Memory Leaks
`QFile* statusFileObj` is allocated with `new` inside the loop but never deleted. Should use stack allocation or `delete` after use.

### 2. Fragile /proc Parsing
- The code assumes a fixed line offset to find `Uid:` (8 `readLine()` calls after the first). If `/proc/[pid]/status` format changes, this breaks silently.
- The `Name:` line is read without verifying it actually starts with "Name:".
- The `Uid:` line is read without verifying format. A proper parser should search for the correct label prefix.

### 3. Non-Standard Qt6 Includes
Includes use `<qt6/QtWidgets/QApplication>` style paths. This works with the CMake setup but is non-standard.

### 4. Incomplete Metrics Screen
The Metrics screen is just a black `QLabel` placeholder. The Processes screen is commented out during creation but then properly instantiated as `ProcessWidget`.

### 5. No Error Handling
- No handling for unreadable `/proc/[pid]/status` files (permission denied, zombie processes, etc.)
- No validation that extracted values are valid numbers/names
