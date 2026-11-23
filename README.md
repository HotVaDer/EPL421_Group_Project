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

## Presentation Outline

**Duration:** about 30 minutes

| Part | Sections | Time |
|------|----------|------|
| 1 – The problem: traditional I/O | 1–3 | ~6 min |
| 2 – The solution: io_uring and liburing | 4–6 | ~9 min |
| 3 – In practice | 7–10 | ~9 min |
| 4 – Conclusions | 11 | ~6 min |

### Part 1 – The problem: traditional I/O

1. **What happens when a program does I/O:** the system call and the switch from user mode to kernel mode
2. **Blocking I/O:** what happens with three consecutive `read()` calls, each one waiting for the previous to finish
3. **A classic web server with `read`/`write`:** how it serves clients, and the limits of this model when there are many users

### Part 2 – The solution: io_uring and liburing

4. **What io_uring is:** an asynchronous I/O interface built on memory shared between the application and the kernel
5. **Submission and Completion Queues:** the SQ and CQ rings, and what an SQE and a CQE are
6. **The liburing API:** a simple example that submits multiple reads at once

### Part 3 – In practice

7. **Where liburing wins:** a web server scenario where io_uring performs better
8. **Benchmarks and results**
9. **`SQPOLL`:** a kernel thread that watches the submission queue, so submitting work needs no system call
10. **Where liburing loses:** a scenario where io_uring performs worse

### Part 4 – Conclusions

11. **Conclusions:**
    - Advantages and limitations
    - Drawbacks of liburing
    - Security problems
    - History of io_uring: how it evolved, the problems it faced and the exploits found against it

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
