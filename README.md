# Example Output (Server)

Performance counter stats for './uds-server':

    37      syscalls:sys_enter_read
     3      syscalls:sys_enter_write
     2      syscalls:sys_enter_accept
    
    97 seconds time elapsed

# Server-Side Interpretation
Performance counter stats for './uds-server':

| Server-Side Interpretation | Observation                                      | Meaning                                     | Inference                                    |
|----------------------------|------------------------------------------------|---------------------------------------------|----------------------------------------------|
| read = 37                  | The server performed 37 read operations from connected clients.  | Low load; likely few requests received.    | Server experienced minimal incoming data requests. |
| write = 3                  | Only 3 responses sent to clients.               | Indicates limited interaction or mostly idle server. | Server mostly idle with few outbound responses.     |
| accept = 2                 | Two client connections accepted.                 | Few clients connected during profiling.    | Low number of unique client connections.            |
| Overall                    | Low syscall counts and high elapsed time (97s)  | Server was mostly waiting for client requests or idle. | Server mainly in wait state, low activity load.     |

# Example Output (Client)

 Performance counter stats for './uds-client':

        6      syscalls:sys_enter_read
        9      syscalls:sys_enter_write
        0      syscalls:sys_enter_accept

   57 seconds time elapsed

   # Client-Side Interpretation

   Client-Side Interpretation

   | Observation       | Meaning                                      | Inference                                  |
|-------------------|----------------------------------------------|--------------------------------------------|
| write = 9       | Client issued many write calls (sending commands/requests). | Indicates client actively communicating with the server. |
| read = 6        | Client received responses fewer times than it sent data.   | May be waiting for replies or batching requests.           |
| connect = 0      | Two connection attempts made.                  | Could be reconnects or multiple sessions.                   |
| Overall           | More writes than reads                          | Client is request-heavy; may be generating load.           |

# 🧩 Syscall Bottleneck Interpretation Summary

This table summarizes how to interpret `perf stat` syscall counters for a Unix Domain Socket (UDS) server and provides optimization strategies for each case.

| **Syscall** | **When Count is High** | **Likely System State** | **Possible Fix / Optimization** |
|--------------|------------------------|---------------------------|----------------------------------|
| **read()** | Frequent small reads | I/O-bound, waiting for data | - Use larger read buffers (4KB–8KB)<br>- Batch multiple reads together<br>- Switch to non-blocking I/O with `epoll()`<br>- Use `readv()` or `recvmsg()` to read multiple buffers at once<br>- Use `mmap()` for file-backed reads |
| **write()** | Many small writes | Kernel overhead due to frequent syscall transitions | - Batch or buffer writes before sending<br>- Use `writev()` / `sendmsg()` for combined writes<br>- Enable socket-level buffering (disable `TCP_NODELAY` unless necessary)<br>- Use asynchronous I/O (`aio_write`) |
| **accept()** | Many short-lived connections | High connection churn, frequent process/thread creation | - Reuse persistent connections where possible<br>- Implement connection pooling<br>- Use thread or process pools to handle clients<br>- Switch to event-driven design (`epoll()`) |
| **Blocking I/O** | Server threads often waiting | CPU idle while blocked on `read` or `write` | - Set non-blocking flag (`O_NONBLOCK`)<br>- Use I/O multiplexing (`select`, `poll`, or `epoll`)<br>- Implement event-driven architecture<br>- Use asynchronous or deferred I/O |
| **Context Switching** | Frequent user↔kernel transitions | High syscall overhead reducing throughput | - Reduce syscall frequency via batching<br>- Combine read/write operations<br>- Optimize data copy between user and kernel space |

---

## 📊 Example perf stat Usage

```bash
sudo perf stat -e syscalls:sys_enter_read,syscalls:sys_enter_write,syscalls:sys_enter_accept ./uds-server



   

   
