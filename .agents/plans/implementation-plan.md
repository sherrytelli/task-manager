# Task Manager - Implementation Plan

## Phase 1: Bug Fixes & Code Quality

### 1.1 Fix Memory Leaks in `ProcessWidget`
**File**: `src/processeswidget.cpp`
**Issue**: `QFile* statusFileObj` allocated with `new` inside loop, never deleted.
**Action**: Replace heap allocation with stack allocation (`QFile statusFileObj` instead of `new QFile`).

### 1.2 Fix Fragile /proc Parsing
**File**: `src/processeswidget.cpp`
**Issue**: `Uid:` line is found by skipping a fixed number of lines (8). This breaks if `/proc/[pid]/status` format changes.
**Action**: Parse the `status` file line by line, searching for lines starting with `Name:` and `Uid:` labels. This makes the parser robust to format changes.

### 1.3 Add Input Validation
**File**: `src/processeswidget.cpp`
**Issue**: No validation that extracted values are valid. Zombie processes (no `status` file) cause silent failures.
**Action**: 
- Check if `Name:` and `Uid:` lines exist before parsing
- Skip directories that don't correspond to valid processes (e.g., non-numeric PIDs if any slip through)
- Handle permission-denied errors gracefully

### 1.4 Fix Include Style
**Files**: All `.h` and `.cpp` in `src/`
**Issue**: Non-standard `<qt6/QtWidgets/...>` includes.
**Action**: Replace with standard Qt6 includes (e.g., `<QApplication>`, `<QMainWindow>`, `<QVBoxLayout>`, etc.). CMake's `find_package(Qt6 ...)` with `qt_standard_project_setup()` handles the paths.

---

## Phase 2: Enhanced Process Table

### 2.1 Add More Process Columns
**File**: `src/processeswidget.h`, `src/processeswidget.cpp`
**Columns to add**:
| Column | Source | Description |
|---|---|---|
| CPU% | `/proc/[pid]/stat` fields 13-14 | CPU time used (utime + stime) |
| Memory% | `/proc/[pid]/statm` or `/proc/[pid]/status` VmRSS | Resident memory in KB |
| State | `/proc/[pid]/status` Status: line | Running, sleeping, zombie, etc. |
| Start Time | `/proc/[pid]/stat` field 22 | Process start time |
| Thread Count | `/proc/[pid]/status` Threads: | Number of threads |
| Command Line | `/proc/[pid]/cmdline` (null-separated) | Full command with arguments |

**Implementation**:
- Update `setColumnCount()` to include new columns
- Update `setHorizontalHeaderLabels()` with new headers
- In `updateProcessesList()`, read additional `/proc/[pid]/` files for each process
- Calculate CPU% relative to total system CPU time (read `/proc/stat` for system-wide totals)
- Calculate Memory% relative to total system memory (read `/proc/meminfo`)

### 2.2 Column Resizing & Widths
**File**: `src/processeswidget.cpp`
**Action**: 
- Set sensible default column widths
- Use `setSectionResizeMode()` for stretch/fixed columns (e.g., PID fixed, COMMAND stretched)
- Store preferred widths in a config array

### 2.3 Sortable Columns
**File**: `src/processeswidget.h`, `src/processeswidget.cpp`
**Action**:
- Enable `setSortingEnabled(true)` on `QTableWidget`
- Handle numeric sorting for PID, CPU%, Memory% columns (use `Qt::DisplayRole` with numeric values)
- Store numeric values as `QTableWidgetItem::data(Qt::DisplayRole)` for proper sorting
- Add visual indicator (arrow icon) to show sort direction

### 2.4 Process Kill Functionality
**File**: `src/processeswidget.h`, `src/processeswidget.cpp`
**Action**:
- Enable context menu: `tableWidget->setContextMenuPolicy(Qt::CustomContextMenu)`
- On right-click, show `QMenu` with "Kill Process" / "Kill Process (Force)" options
- "Kill Process" sends SIGTERM (`kill(pid, SIGTERM)`)
- "Kill Process (Force)" sends SIGKILL (`kill(pid, SIGKILL)`)
- Show confirmation dialog before killing
- Check for root privileges (if not root, some processes can't be killed)
- Refresh table after kill attempt

### 2.5 Process Details Dialog
**File**: `src/processdetailsdialog.h`, `src/processdetailsdialog.cpp` (new file)
**Action**:
- Create a new `QDialog` that shows detailed process information
- Triggered by double-clicking a row in the table
- Display: PID, Name, Command Line, User, State, CPU%, Memory%, Threads, Start Time, Working Directory, Parent PID, Status file contents
- Non-editable layout with grouped sections

### 2.6 Status Bar
**File**: `src/mainwindow.h`, `src/mainwindow.cpp`
**Action**:
- Add a `QStatusBar` to `MainWindow`
- Show: total process count, system uptime, last refresh time
- Update status bar text on each process refresh (via signal from `ProcessWidget`)

---

## Phase 3: Metrics Screen Implementation

### 3.1 System Overview Card
**File**: `src/metricswidget.h`, `src/metricswidget.cpp` (new file)
**Action**:
- Create a new `MetricsWidget` class (similar to `ProcessWidget`)
- Display system-wide statistics in a card-based layout
- Cards to show:
  - **CPU**: Usage percentage, per-core usage, load average (1m, 5m, 15m)
  - **Memory**: Used vs total, swap usage, buffers, cached
  - **Disk**: Per-mount-point usage (read from `/proc/mounts` + `statvfs`)
  - **Network**: Bytes sent/received per interface (read from `/proc/net/dev`)
  - **Uptime**: System uptime, number of processes, number of threads

### 3.2 CPU Usage Chart
**File**: `src/cpuchart.h`, `src/cpuchart.cpp` (new file)
**Action**:
- Create a custom `QCustomPaint` widget or use `QChart` (QtCharts module) for CPU history
- Show CPU usage over time (last 30 seconds)
- Separate lines for total, user, system, idle
- Update every 1 second
- If QtCharts is not available, implement a custom `QPainter`-based chart

### 3.3 Memory Usage Chart
**File**: `src/memorychart.h`, `src/memorychart.cpp` (new file)
**Action**:
- Similar to CPU chart, but for memory usage
- Show breakdown: used, cached, buffered, free, swap
- Update every 2 seconds

### 3.4 Disk I/O Monitor
**File**: Part of `MetricsWidget` or separate `DiskChart` widget
**Action**:
- Show per-disk read/write speeds
- Read from `/proc/diskstats`
- Calculate delta between readings

### 3.5 Network Monitor
**File**: Part of `MetricsWidget` or separate `NetworkChart` widget
**Action**:
- Show per-interface bytes sent/received
- Read from `/proc/net/dev`
- Calculate delta between readings

---

## Phase 4: UI/UX Improvements

### 4.1 Menu Bar
**File**: `src/mainwindow.h`, `src/mainwindow.cpp`
**Action**:
- Add a `QMenuBar` with:
  - **File**: Refresh Now, Exit
  - **View**: Toggle Columns (PID, CPU%, Memory%, etc.), Refresh Interval (1s, 2s, 5s, 10s)
  - **Help**: About, Documentation

### 4.2 Refresh Interval Setting
**File**: `src/processeswidget.h`, `src/processeswidget.cpp`
**Action**:
- Make refresh interval configurable (currently hardcoded to 2000ms)
- Add a method `setRefreshInterval(int ms)` on `ProcessWidget`
- Connect to a menu action or toolbar dropdown
- Persist setting in `QSettings`

### 4.3 Search Enhancement
**File**: `src/processeswidget.h`, `src/processeswidget.cpp`
**Action**:
- Expand search to match across all columns (PID, USER, COMMAND, STATE, etc.)
- Add a "Clear Search" button next to the search field
- Highlight matching text in the table (optional, via custom delegate)

### 4.4 Column Visibility Toggle
**File**: `src/mainwindow.h`, `src/mainwindow.cpp`
**Action**:
- Allow user to show/hide columns via View menu
- Store visible columns in `QSettings`
- Reapply on app restart

### 4.5 Theme / Styling
**File**: `src/mainwindow.cpp`, `src/processeswidget.cpp`
**Action**:
- Apply a consistent stylesheet across the application
- Consider dark/light theme support (detect system preference via `QStyleHints`)
- Style the table with alternating row colors, hover effects

### 4.6 Window State Persistence
**File**: `src/mainwindow.h`, `src/mainwindow.cpp`
**Action**:
- Save window geometry and state via `QSettings`
- Restore on next launch

---

## Phase 5: Settings & Configuration

### 5.1 Settings Dialog
**File**: `src/settingsdialog.h`, `src/settingsdialog.cpp` (new file)
**Action**:
- Create a `QDialog` for application settings
- Settings to include:
  - Refresh interval (dropdown: 1s, 2s, 5s, 10s, 30s)
  - Default columns and order
  - Theme (light/dark/system)
  - Auto-start with system (optional, Linux desktop entry)
  - Kill confirmation toggle (always confirm before killing)

### 5.2 Settings Persistence
**File**: `src/mainwindow.h`, `src/mainwindow.cpp`
**Action**:
- Use `QSettings` to save/load settings
- Load settings on startup
- Save settings on dialog accept

---

## Phase 6: Polish & Testing

### 6.1 Keyboard Shortcuts
**File**: `src/mainwindow.h`, `src/mainwindow.cpp`
**Action**:
- Ctrl+R: Refresh now
- Ctrl+F: Focus search bar
- Ctrl+K: Kill selected process
- Alt+M: Switch to Metrics tab
- Alt+P: Switch to Processes tab
- Escape: Clear search / close dialogs

### 6.2 Error Handling & Logging
**File**: All source files
**Action**:
- Centralize error handling (e.g., a helper class or utility functions)
- Log errors to console or a log file
- Show user-friendly error messages via `QMessageBox`

### 6.3 Edge Cases
**File**: `src/processeswidget.cpp`
**Action**:
- Handle rapidly changing process list (processes appearing/disappearing during iteration)
- Handle very large process counts (1000+ processes) efficiently
- Handle non-UTF8 filenames in command lines
- Handle processes that change UID mid-execution

### 6.4 Accessibility
**File**: All UI files
**Action**:
- Add accessible names to widgets
- Ensure tab order is logical
- Support screen readers

---

## File Change Summary

### New Files to Create
| File | Purpose |
|---|---|
| `src/processdetailsdialog.h` / `.cpp` | Process details dialog |
| `src/metricswidget.h` / `.cpp` | Metrics screen widget |
| `src/cpuchart.h` / `.cpp` | CPU usage chart (custom painter or QtCharts) |
| `src/memorychart.h` / `.cpp` | Memory usage chart |
| `src/settingsdialog.h` / `.cpp` | Settings dialog |

### Files to Modify
| File | Changes |
|---|---|
| `src/processeswidget.h` | Add new methods, signals, member variables |
| `src/processeswidget.cpp` | Fix parsing, add columns, sorting, kill, search |
| `src/mainwindow.h` | Add menu bar, status bar, settings members |
| `src/mainwindow.cpp` | Implement menu, status bar, settings integration |
| `CMakeLists.txt` | Add new source files, optional QtCharts component |

---

## Suggested Implementation Order

1. **Phase 1** (Bug fixes) — Quick wins, reduces technical debt
2. **Phase 4.1** (Menu bar) — Enables user control for subsequent features
3. **Phase 2** (Enhanced process table) — Core functionality improvement
4. **Phase 3** (Metrics screen) — Adds new feature area
5. **Phase 4.2–4.6** (UI/UX polish) — Improves usability
6. **Phase 5** (Settings) — Configuration persistence
7. **Phase 6** (Polish) — Keyboard shortcuts, edge cases, accessibility
