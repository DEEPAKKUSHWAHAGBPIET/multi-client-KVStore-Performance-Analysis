---

## 🧩 Case 1: High Volume of `read()`

### What it means
The server is doing many small `read()` calls, each reading just a little bit of data.

### Why this is a problem
- Every `read()` causes a switch from user to kernel mode, which is slow.  
- Small reads don’t allow the system to group data efficiently.  
- This increases CPU work because of more context switches.

### How to fix it

#### 1. Use bigger buffers  
Instead of reading just a few bytes, read larger chunks, like 4KB or 8KB.


Fewer reads means fewer syscalls.

#### 2. Use non-blocking I/O with `select()` or `epoll()`  
Don’t wait (block) for data. Instead, check if data is ready before reading. This helps manage many clients better.

#### 3. Use `readv()` or `recvmsg()`  
These let you read into multiple buffers in one syscall, lowering overhead.

#### 4. Use memory mapping (`mmap`)  
For files, map them to memory and read from there without syscalls after mapping.

---

## 🧩 Case 2: High Volume of `write()`

### What it means
The server sends many small writes, like tiny responses per client.

### Why this is bad
- Each `write()` causes CPU to switch contexts.  
- Sending small data frequently slows things down due to flushing buffers too often.

### How to fix it

#### 1. Buffer writes  
Collect data in a buffer, then send it all at once.


#### 2. Use `writev()` or `sendmsg()`  
Send multiple buffers in one syscall.

#### 3. Enable socket buffering smartly  
Use TCP_NODELAY only if low latency is very important. Otherwise, let the system batch the data.

#### 4. Use asynchronous I/O (`aio_write`)  
This lets the program continue running while the data is written in the background.

---

## 🧩 Case 3: Blocking Calls (`read()`, `write()`, `accept()`)

### What it means
The server process or thread is waiting and not doing any useful work.

### Why it’s bad
- CPU sits idle waiting.  
- Limits how many clients can be handled at once.  
- The server might feel slow or unresponsive during heavy use.

### How to fix it

#### 1. Use non-blocking mode  
Set file descriptors to non-blocking:


Then `read()` or `write()` returns immediately if data isn't there.

#### 2. Use I/O multiplexing  
Use `select()`, `poll()`, or `epoll()` to handle many connections in one thread without blocking.

#### 3. Use thread or process pools  
Create worker threads or processes beforehand to do blocking operations together, avoiding overhead from creating new ones.

---

## ⚙️ Summary Table

| Problem            | Cause           | Fixes                             |
|--------------------|-----------------|----------------------------------|
| High `read()` calls | Small data reads | Bigger buffers / `readv()` / `mmap` |
| High `write()` calls| Many small writes| Batch writes / `writev()`         |
| Blocking syscalls   | Waiting on I/O  | Use non-blocking + `epoll()`      |

---
