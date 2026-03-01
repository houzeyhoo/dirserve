# dirserve
dirserve is a simple asynchronous static HTTP/1.0 server for Linux, written in modern C++. It serves static files 
using non‑blocking I/O and the `epoll` API.

The idea behind the project was to create a simple tool that lets you conveniently serve a directory of files via HTTP,
while maintaing a level of performance. It was made mostly for fun and learning purposes. Think of it as a replacement
for running `python -m http.server`.

> Note: this repository contains a cleaned‑up version of an old personal project. The original Git history and detailed 
comments were removed for privacy reasons (they contained personal information... also, they were in Polish). The core 
design and implementation are preserved here. At some point, I'll likely go over the original repository and see about
translating comments.

## Features
- Multi‑threaded architecture with per‑thread event loops and `SO_REUSEPORT`‑based accept load distribution.
- Non‑blocking TCP I/O built on `epoll` (edge-triggered) for handling many concurrent connections efficiently.
- Static file serving over HTTP/1.0 with support for common MIME types.
- Basic security for serving static content:
	- path traversal prevention using `openat2` with `RESOLVE_IN_ROOT`,
	- request size limits and simple timeouts.
- A basic logging system with synchronous and asynchronous backends.

In local benchmarks using [wrk](https://github.com/wg/wrk) (8 threads, 10000 connections) on a multi‑core machine, 
dirserve running with 8 subservers handled about ~100k requests/second for small static files (see the demo directory).

## Build

Requirements (example on a recent Linux distribution):

- Linux kernel 3.9 or newer
- CMake >= 3.22
- A C++20 compiler (e.g. GCC >= 11 or Clang with libstdc++)
- Make or another CMake generator

The only third‑party dependency is [fmtlib](https://github.com/fmtlib/fmt) (fetched automatically at configure time).

Typical build instructions:
```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## Usage

Run the server from the build directory, configuring it via command‑line flags:

```bash
./dirserve -s <workers> -p <port> [-c <max_clients>] [-o <log_file>] [-d <doc_root>]
```

Key options (see `src/utils/options.hpp` for details):

- `-s` - number of subservers (worker threads). For best performance, use up to the number of logical CPU cores. (Required)
- `-p` - TCP port to listen on. (Required)
- `-c` - maximum number of concurrent clients per worker. Default: 65535.
- `-o` - path to a log file. If omitted, logs are written to stdout.
- `-d` - document‑root directory for static files. Default: `/var/www/dirserve/` (must already exist).
- `-h` - print a help message and exit (currently, there's only a placeholder).

For high connection counts, you should raise the per‑process file descriptor limit like so:
```bash
ulimit -n 50000
```

## Architecture overview
- The main process parses options, sets up logging, and installs basic signal handling.
- A top‑level `http::server` creates multiple subservers (workers), each running on its own thread.
- Each subserver owns:
	- its own listening socket on the same port (using `SO_REUSEPORT` for kernel‑level load distribution),
	- its own `epoll` instance for event handling,
	- a map of active HTTP transactions (a "transaction" here means a request/response HTTP/1.0 exchange).
- The server uses `openat2` with a directory file descriptor pointing at the document root to safely open files without allowing directory traversal above that root.

HTTP/1.1 features (keep-alive, pipelining, etc.) are currently not implemented, but I may come back to this project in
the future and add those features eventually.
