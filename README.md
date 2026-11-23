# liburing – Programming Linux with io_uring

Group presentation for **EPL421 – Systems Programming**, University of Cyprus.

This repository holds the slides, notes and example programs for our presentation on
**io_uring**, the asynchronous I/O interface of the Linux kernel, and **liburing**, the
user-space library that makes it practical to use.

## Team

| Name | Role / Topics |
|------|---------------|
| Giorgos Papapetrou | _TBD_ |
| Andreas Michail | _TBD_ |
| Stylianos Kyriakou | _TBD_ |

## What is io_uring?

`io_uring` was added in **Linux 5.1** (2019, by Jens Axboe) as a faster, more general
replacement for older asynchronous I/O interfaces such as POSIX AIO and Linux `aio`.
It works through two ring buffers that the kernel and the application share in memory:

- **Submission Queue (SQ):** the application places I/O requests (*SQEs*) here.
- **Completion Queue (CQ):** the kernel posts results (*CQEs*) here.

Because the rings are shared, an application can submit many operations and collect
many results with few system calls, or with none at all when submission polling
(`SQPOLL`) is enabled.

The raw kernel interface consists of three system calls: `io_uring_setup(2)`,
`io_uring_enter(2)` and `io_uring_register(2)`. **liburing** wraps these and the ring
memory handling in a simpler C API.

## Presentation Outline (draft)

1. Motivation: the cost of blocking I/O and system calls, and the limits of `select`/`poll`/`epoll` and Linux AIO
2. io_uring architecture: SQ/CQ rings, SQEs, CQEs and the three system calls
3. The liburing API: setup, submitting work, reaping completions, cleanup
4. Live examples: file I/O, then a simple network server
5. Advanced features: linked requests, registered files and buffers, `SQPOLL`, multishot operations
6. Performance comparison with traditional I/O
7. Security considerations and where io_uring is restricted
8. Conclusions and Q&A

## Basic liburing Workflow

```c
#include <liburing.h>

struct io_uring ring;
io_uring_queue_init(8, &ring, 0);            // create SQ/CQ with 8 entries

struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
io_uring_prep_read(sqe, fd, buf, len, 0);     // describe the operation
io_uring_submit(&ring);                       // hand it to the kernel

struct io_uring_cqe *cqe;
io_uring_wait_cqe(&ring, &cqe);               // wait for the result
/* cqe->res holds the return value (bytes read or -errno) */
io_uring_cqe_seen(&ring, cqe);                // mark the completion as consumed

io_uring_queue_exit(&ring);
```

## Repository Structure

```
.
├── README.md
├── .gitignore
├── Dockerfile           # Ubuntu 24.04 + liburing dev environment
├── docker-compose.yml   # runs the container with io_uring allowed
├── .devcontainer/       # VS Code Dev Containers config
├── slides/        # presentation slides
├── docs/          # research notes and references
└── examples/      # C example programs using liburing
    └── Makefile
```

## Development Environment (Docker – recommended)

io_uring is a **Linux-only** interface. To work on macOS or Windows, use the Docker
container in this repo. It runs Ubuntu 24.04 with `liburing-dev`, `gcc`, `make`, `gdb`,
`strace` and `valgrind` installed.

Requires [Docker Desktop](https://www.docker.com/products/docker-desktop/) (running).

```sh
# first time (or after changing the Dockerfile)
docker compose build

# open a Linux shell inside the container (starts in examples/)
docker compose run --rm dev

# inside the container
make
./hello_uring
```

The repo folder is mounted at `/workspace`, so you can edit files on your host and
compile and run them inside the container.

**VS Code:** with the *Dev Containers* extension installed, run
**Dev Containers: Reopen in Container** to work directly inside the Linux environment.

> **Why `seccomp=unconfined`?** Docker's default seccomp profile blocks the io_uring
> system calls, and `io_uring_queue_init` then fails with `Operation not permitted`.
> `docker-compose.yml` disables seccomp for this container so io_uring works.
> If you use `docker run` directly, add `--security-opt seccomp=unconfined`.

## Requirements (native Linux)

To work without Docker, you need a Linux machine (or VM) with:

- Linux kernel **5.1+** (5.10+ recommended; newer kernels support more operations)
- GCC or Clang, and `make`
- liburing development package:

```sh
# Debian / Ubuntu
sudo apt install liburing-dev

# Fedora
sudo dnf install liburing-devel

# Arch
sudo pacman -S liburing
```

Or build it from source:

```sh
git clone https://github.com/axboe/liburing.git
cd liburing && ./configure && make && sudo make install
```

## Building and Running the Examples

```sh
cd examples
make
./<example_name>
```

or compile a single file by hand:

```sh
gcc -Wall -O2 example.c -o example -luring
```

> **Note:** Some environments disable io_uring for security reasons, for example
> container seccomp profiles or the `kernel.io_uring_disabled` sysctl (Linux 6.6+).
> If a call fails with `EPERM` or `ENOSYS`, check these settings first.

## References

- liburing source and examples: <https://github.com/axboe/liburing>
- Jens Axboe, *Efficient IO with io_uring*: <https://kernel.dk/io_uring.pdf>
- *Lord of the io_uring* guide: <https://unixism.net/loti/>
- Man pages: `man io_uring`, `man io_uring_setup`, `man io_uring_enter`, `man io_uring_queue_init`
- LWN.net articles on io_uring: <https://lwn.net/Kernel/Index/#io_uring>

## License

For academic use as part of EPL421 coursework.
