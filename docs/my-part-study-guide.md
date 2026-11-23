# Study Guide – io_uring Presentation & My Part (Part 3)

> Personal study notes for Part 3.

---

## 1. What is the whole presentation about?

**Topic:** how a Linux program can do input/output (I/O) faster using **io_uring**
and the helper library **liburing**.

**Length:** about 30 minutes, in 4 parts:

| Part | What it covers | Time |
|------|----------------|------|
| 1 – The problem | Normal I/O is slow when there are many users | ~6 min |
| 2 – The solution | What io_uring is and how to use it with liburing | ~9 min |
| 3 – In practice (**mine**) | When it is better, numbers, SQPOLL, when it is worse | ~9 min |
| 4 – Conclusions | Pros, cons, security, history | ~6 min |

### The story in one minute

1. **Part 1:** Every `read()` or `write()` is a *system call*. The program stops, jumps
   into the kernel, waits, and comes back. With thousands of users, a server spends
   a lot of time waiting and switching.
2. **Part 2:** io_uring gives the program and the kernel **two shared queues** in memory:
   - **SQ (Submission Queue):** the program writes requests here ("please read this").
     Each request is an **SQE**.
   - **CQ (Completion Queue):** the kernel writes results here ("done, 512 bytes").
     Each result is a **CQE**.

   The program can put **many requests** in the queue and tell the kernel **once**.
   liburing makes this easy to code.
3. **Part 3 (you):** Show *when* this actually helps, *prove it with numbers*,
   show the extra trick **SQPOLL**, and be honest about *when it does not help*.
4. **Part 4:** Wrap up: good sides, bad sides, security problems, history.

**What the audience already knows when you start:** what blocking I/O is, what
SQ/CQ/SQE/CQE are, and the basic liburing calls. You don't need to explain those again.

---

## 2. My part: Part 3 – "In practice" (~9 min)

Four sections. Suggested time:

| # | Section | Time |
|---|---------|------|
| 7 | Where liburing wins (web server scenario) | ~2.5 min |
| 8 | Benchmarks and results | ~2.5 min |
| 9 | SQPOLL | ~2 min |
| 10 | Where liburing loses | ~2 min |

---

### Section 7 – Where liburing wins

**Main idea:** io_uring helps most when a server has **many clients at the same time**
and does **many small I/O operations**.

#### The scenario

A web server with **thousands of connected clients**. Each client sends small requests
("give me this page"), and the server reads a file and sends it back.

#### Why the classic way struggles

- **Blocking server (one thread per client):** 10,000 clients = 10,000 threads.
  Lots of memory, and the CPU spends time switching between threads.
- **epoll server (the usual "fast" way today):** one thread asks the kernel
  "which clients are ready?". But then it still has to call `read()` / `write()` for
  each one. That is **at least 2 system calls per operation**: one to ask, one to do.
- **epoll does not help with files on disk.** For regular files, epoll always says
  "ready", so a slow disk read still blocks the whole server.

#### Why io_uring does better

| Problem | io_uring answer |
|---------|-----------------|
| Many system calls | Put many requests in the SQ, submit them with **one** call (*batching*) |
| "Is it ready?" + "do it" = 2 steps | io_uring does the work and tells you when it is **done**: 1 step |
| Disk reads block the server | io_uring does real **asynchronous** file reads |
| Accepting each new client = 1 call | **Multishot accept**: one request keeps accepting many clients |

**Simple analogy:** epoll is like a waiter who asks each table "are you ready to order?"
and then walks back to take the order. io_uring is like tables writing orders on a list
and the kitchen putting finished plates on a counter. The waiter just collects.

**Why system calls matter so much:** each one switches from user mode to kernel mode.
Since the Spectre/Meltdown security fixes (2018), this switch costs even more. Fewer
switches = more time for real work.

---

### Section 8 – Benchmarks and results

**Main idea:** show with real numbers that the claims from Section 7 are true.

#### What to compare

- Classic server (blocking or epoll with `read`/`write`)
- io_uring server (with liburing)
- Same machine, same work, same number of clients

#### What to measure

| Metric | Meaning | Better is |
|--------|---------|-----------|
| **Requests per second** (throughput) | How much work the server finishes | Higher |
| **Latency** (average and p99) | How long one client waits. p99 = the slowest 1% | Lower |
| **System calls** | How many times we entered the kernel (`strace -c`) | Fewer |
| **CPU usage** | How hard the machine works for that result | Lower |

#### How to present the results

- One graph: **requests/sec vs number of clients** (e.g. 10, 100, 1,000, 10,000).
  Expected shape: at few clients both are similar; as clients grow, io_uring
  stays higher.
- One table: **system call counts** for the same job. This is the clearest proof,
  because it shows *why* io_uring is faster.

#### Be careful

- **Do not invent numbers.** Only show what we actually measured.
- Running inside **Docker on a Mac** adds a virtual machine layer, so the numbers can be
  less accurate. Say this on the slide if we test there.
- Run each test several times and use the average.

---

### Section 9 – SQPOLL

**Main idea:** with SQPOLL, the program can submit work **without any system call at all**.

#### How it works (step by step)

1. You create the ring with the flag `IORING_SETUP_SQPOLL`.
2. The kernel starts a **special kernel thread** that keeps watching the SQ.
3. Your program writes an SQE into the shared queue. That's it. **No `io_uring_enter()`.**
4. The kernel thread sees the new request and starts it.
5. You read the result from the CQ as usual.

#### When there is no work

- If the queue stays empty for some time (`sq_thread_idle`, in milliseconds),
  the kernel thread **goes to sleep** to save CPU.
- It sets a flag (`IORING_SQ_NEED_WAKEUP`) to say "I'm asleep".
- Next time you submit, liburing's `io_uring_submit()` sees the flag and makes **one**
  system call to wake it up. You don't need to handle this by hand.

#### The trade-off

| Good | Bad |
|------|-----|
| Almost zero system calls under heavy load | The thread **uses a whole CPU core** while it is spinning |
| Lowest latency | Wastes CPU when the load is low |
| | Before Linux 5.11 it needed root privileges |

**Simple analogy:** instead of ringing a bell every time you have an order, a cook stands
at the window and watches the list all the time. Very fast, but that cook does nothing
else.

---

### Section 10 – Where liburing loses

**Main idea:** io_uring is **not always** better. Being honest here makes the
presentation stronger.

#### Cases where it is worse (or not worth it)

1. **Few clients, low load.** With 5 users there are few system calls to save.
   A simple blocking server is just as fast and much easier to write.
2. **SQPOLL with little work.** The polling thread uses a CPU core for nothing.
   On a small machine (2 cores), this steals CPU from the real program.
3. **The bottleneck is the CPU, not I/O.** If each request needs heavy calculation
   (e.g. image processing), saving system calls changes almost nothing.
4. **Operations that fall back to worker threads.** Some operations can't be done
   asynchronously inside the kernel, so the kernel hands them to its own thread pool
   (**io-wq**). Then there is extra overhead, and it can be slower than a normal
   thread pool.
5. **Harder code.** Buffers must stay valid until the CQE arrives. Results come back
   in any order. Bugs are easier to make and harder to find.
6. **Sometimes not allowed.** Docker blocks it by default (seccomp), and Linux 6.6+
   has the `kernel.io_uring_disabled` setting. Some companies (e.g. Google) and
   Android turn it off for security. This leads into Part 4.

#### A possible demo scenario

Same two servers as Section 8, but with **only a few clients**: the numbers are almost
equal. Then turn on **SQPOLL** with low load and show the **high CPU usage** for no gain.

---

## 3. How my part connects to the others

- **From Part 2 to me:** Part 2 ends with a simple liburing example (multiple reads).
  I start with: *"OK, we know how it works. Now, when does it really matter?"*
- **From me to Part 4:** I end with the cases where it loses, including that it is
  sometimes disabled for security. Part 4 starts with: *"So why is it disabled?
  Let's talk about security and history."*

---

## 4. Words to know

| Word | Simple meaning |
|------|----------------|
| **System call** | The program asks the kernel to do something for it |
| **User mode / kernel mode** | Normal program area / the protected kernel area |
| **Blocking** | The program stops and waits until the operation finishes |
| **Asynchronous** | Start the operation, keep working, get the result later |
| **SQ / SQE** | Submission Queue / one request inside it |
| **CQ / CQE** | Completion Queue / one result inside it |
| **Batching** | Sending many requests together with one call |
| **epoll** | Older Linux way to wait for many sockets at once |
| **Throughput** | How much work is done per second |
| **Latency** | How long one request waits |
| **p99** | The time that 99% of requests are faster than |
| **SQPOLL** | Kernel thread that watches the SQ, so no submit call is needed |
| **io-wq** | Kernel's thread pool for operations that can't run fully async |
| **Multishot** | One request that produces many results (e.g. accepts many clients) |

---

## 5. Things to check / learn more

- [ ] Try `strace -c` on a simple program to see system call counts
- [ ] Read the SQPOLL part of *Lord of the io_uring*: <https://unixism.net/loti/tutorial/sq_poll.html>
- [ ] `man io_uring_setup`: look at `IORING_SETUP_SQPOLL` and `sq_thread_idle`
- [ ] Agree with the team which servers and tools we use for the benchmarks
