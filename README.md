# Concurrent FTP Client

A concurrent FTP client for Linux implementing **RFC 959**. It uses a **multi-process architecture** (via `fork()`) to handle file transfers in the background, keeping the shell prompt active for other commands.

## Implemented Commands

Implemented according to the main project objective:
- **USER**, **PASS**, **STOR**, **RETR**, **PORT**, **PASV**

Additional commands implemented:
- **MKD**, **PWD**, **LIST**, **DELE**, **CWD**, **QUIT**

## Key Features

  * **Concurrency:** Non-blocking uploads/downloads.
  * **Modes:** Supports **Passive (PASV)** (default) and **Active (PORT)**.
  * **Resuming:** `restart <bytes>` allows resuming interrupted transfers.
  * **File Management:** `ls`, `cd`, `mkdir`, `delete`, `pwd`.
  * **Modular C:** Built on Linux Socket API.

## Build & Run

**Prerequisites:** Linux, GCC, Make.

```bash
# 1. Compile
make

# 2. Run
./MartinezK_clienteFTP <server_ip>
# Example: ./MartinezK_clienteFTP 127.0.0.1
```

To clean build files: `make clean`

## Commands Reference

Enter credentials when prompted. Once inside (`ftp>`), use:

| Category | Commands | Description |
| :--- | :--- | :--- |
| **Transfer** | `get <file>`, `put <file>` | Download / Upload files. |
| | `restart <bytes>` | Start next transfer from byte offset. |
| **Navigation** | `ls`, `pwd`, `cd <path>` | List, show path, change dir. |
| **Actions** | `mkdir <name>`, `delete <file>` | Create dir / Delete file. |
| **System** | `passive` | Toggle PASV / PORT mode. |
| | `help`, `quit` | Show help / Exit. |

-----

*Author: Kevin Martínez | [@Al3xMR](https://github.com/Al3xMR)*
