# INTERM - Next-Generation Terminal Editor

**INTERM** (In-Terminal) is a modern, high-performance, modal terminal code editor built for speed, extensibility, and the AI era. It is engineered as an "editor operating system" rather than a simple text container.

## 🚀 Vision

Interm combines the efficiency of modal editing (Helix/Vim) with the power of modern IDE architectures (VSCode/IntelliJ). It is designed from the ground up to be:

- **Async-First:** No UI-blocking operations (LSP, AI, IO, and Syntax parsing are always background tasks).
- **Extreme Performance:** Target cold start < 20ms.
- **AI-Native:** Modular AI integration with deep context awareness.
- **Extensible:** A secure Python-based plugin sandbox.

## 🏗️ Architecture

INTERM follows a modular, layered architecture implemented in **C** for the core and **Python** for orchestration.

### Core Pillars
- **Piece Table Buffer:** Efficient text storage supporting large files, multi-cursor editing, and instant snapshots.
- **Async Runtime:** A custom thread-pool task scheduler for non-blocking execution.
- **Event Bus:** A decoupled communication backbone connecting all subsystems.
- **Diff-based Renderer:** High-performance incremental rendering that minimizes ANSI escape sequences.
- **Deterministic State Machine:** A centralized "source of truth" for modes, cursors, and system state.

## 🛠️ Getting Started

### Prerequisites
- GCC or Clang
- POSIX-compliant environment (Linux/macOS)
- Pthreads library

### Building
```bash
make
```

### Running
```bash
./interm
```

## ⌨️ Basic Usage

INTERM is a modal editor.

- **NORMAL Mode:** (Default) Navigate and execute commands.
  - `i`: Enter **INSERT** mode.
  - `:`: Enter **COMMAND** mode.
  - `h`, `l`: Move cursor left/right.
- **INSERT Mode:** Direct text entry.
  - `Esc`: Return to **NORMAL** mode.
- **COMMAND Mode:** Execute editor commands.
  - `:q`: Quit.
  - `:q!`: Force quit.
  - `:w`: Save (Stub).
  - `Esc`: Cancel command.

## 📂 Project Structure

- `core/`: State machine, boot system, event bus, and task scheduler.
- `buffer/`: Piece Table implementation and text manipulation.
- `render/`: Terminal grid and diff-based rendering logic.
- `terminal/`: Low-level TTY handling and raw mode.
- `include/`: Shared headers and API definitions.
- `docs/`: Architectural specifications and vision documents.

## 📜 License
Internal Project - All rights reserved.
# Interm
