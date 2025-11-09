# Concurrent FTP Client

This project is a **concurrent FTP client** written in **C**, using the **Linux kernel socket API** to connect to an FTP server. It implements commands defined by **[RFC 959](https://www.rfc-editor.org/rfc/rfc959)**.

## Features

* Supports **multiple concurrent file transfers** (both upload and download).
* Keeps the **control connection active** while handling multiple data connections.
* Uses concurrency to improve transfer performance.
* Includes helper functions to simplify and optimize socket creation and connection:

  * `connectsock.c`
  * `connectTCP.c`
  * `errexit.c`

## Build Instructions

A **Makefile** is provided to simplify compilation using **GCC**:

```bash
make
```

This command builds the main FTP client executable.

## Usage

After compiling, run the client specifying the FTP server and port:

```bash
./MartinezK_clienteFTP <server> <port>
```

Example:

```bash
./MartinezK_clienteFTP 127.0.0.1 21
```

## Requirements

* Linux operating system
* **GCC** compiler
* Standard C libraries


