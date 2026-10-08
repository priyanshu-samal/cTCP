# 🌐 Socket Programming in C — The Journey from Scratch

> A structured, hands-on roadmap to mastering low-level network programming with Berkeley Sockets in C. Designed as both a personal learning log and a standalone reference guide.

---

## 📑 Index

| Level | Description |
| :--- | :--- |
| [Level 1 (`lev1`)](#-level-1-basic-single-message-tcp-communication) | Single-message blocking TCP server & client |
| [Level 2 (`lev2`)](#-level-2-continuous-bi-directional-echo-session) | Interactive continuous TCP echo loop with error & EOF handling |
| [Level 3 (`lev3`)](#-level-3-tcp-framing-buffer-management--command-parsing) | Stream framing with newline delimitation & buffer accumulation |
| [Level 4 (`lev4`)](#-level-4-multi-process-concurrent-server-with-fork) | Concurrent multi-client architecture using process isolation (`fork`) |
| [Level 5 (`lev5`)](#-level-5-thread-safe-in-memory-key-value-store-pthreads) | Thread-safe concurrent Hash Table with mutex locking (`pthread`) |
| [Level 6 (`lev6`)](#-level-6-high-performance-event-loop-with-linux-epoll) | Non-blocking I/O event multiplexing with Linux `epoll` |

---

## 🧠 The Core Mental Model

In Unix-like systems, **everything is a file**. A socket is simply a special **file descriptor (FD)** managed by the operating system kernel that represents an open network communication endpoint.

When your application interacts with a socket:
1. You ask the OS kernel to allocate networking state buffers and resources.
2. The OS handles all low-level network layers (Ethernet framing, IP routing, TCP packets, checksums, window sizing, retransmission).
3. Your application simply reads from and writes to the kernel socket buffers using standard file-descriptor interfaces (`send`, `recv`, `read`, `write`, `close`).

```
+-------------------------------------------------------------+
|                      User Application                       |
|         read() / recv()              write() / send()       |
+--------------------^--------------------|-------------------+
                     |                    |
=====================| System Call API ===|====================
                     |                    v
+--------------------|----------------------------------------+
| Kernel Space       |                    |                   |
|              [Recv Buffer]        [Send Buffer]             |
|                    ^                    |                   |
|                    |     TCP/IP Stack   v                   |
|              +--------------------------------+             |
|              | TCP / IP Protocol Engine       |             |
|              +--------------------------------+             |
+--------------------^--------------------|-------------------+
                     |                    v
             [Network Interface Card (NIC) / Physical Wire]
```

---

## 🔌 Fundamental Socket API Breakdown

Here is the essential socket lifecycle cheat sheet:

```
          SERVER                                CLIENT
     +---------------+                    +---------------+
     |   socket()    |                    |   socket()    |
     +-------+-------+                    +-------+-------+
             |                                    |
     +-------v-------+                            |
     |    bind()     |                            |
     +-------+-------+                            |
             |                                    |
     +-------v-------+                            |
     |   listen()    |                            |
     +-------+-------+                            |
             |                                    |
     +-------v-------+      TCP 3-Way             |
     |   accept()    |<-------------------+   connect()   |
     +-------+-------+      Handshake     +-------+-------+
             |                                    |
             |  Connected (client_fd <-> sockfd)  |
             +------------------+-----------------+
                                |
                 +--------------v--------------+
                 |  send()  <=====>   recv()   |
                 |  recv()  <=====>   send()   |
                 +--------------+--------------+
                                |
             +------------------+-----------------+
             |                                    |
     +-------v-------+                    +-------v-------+
     |    close()    |                    |    close()    |
     +---------------+                    +---------------+
```

### 1. `socket()`
* **What it is:** Creates an unbound network communication endpoint.
* **Why use it:** Before doing anything on the network, you need a socket file descriptor from the operating system.
* **Under the hood:** The OS kernel allocates internal state structures, buffers (send & receive queues), and assigns an integer file descriptor in your process's FD table.

### 2. `bind()`
* **What it is:** Associates a socket with a specific local address (IP) and port number.
* **Why use it:** Servers need a well-known, predictable port so clients know where to reach them.
* **Under the hood:** The OS registers the port in its routing table for the specified IP interface. If another process is already bound to that port, `bind()` fails with `EADDRINUSE`.

### 3. `listen()`
* **What it is:** Marks a bound socket as *passive* — ready to accept incoming connection requests.
* **Why use it:** By default, sockets are active (initiators). Sockets acting as servers must be switched to listening mode.
* **Under the hood:** The OS creates two queues for this socket: the incomplete connection queue (SYN received) and the completed connection queue (3-way handshake finished, waiting for `accept()`). The `backlog` parameter sets the queue capacity limit.

### 4. `accept()`
* **What it is:** Extracts the first completed connection from the listening queue.
* **Why use it:** To obtain a dedicated socket descriptor dedicated purely to talking with that specific client.
* **Under the hood:** **Critical distinction:** `accept()` returns a **brand new file descriptor** (`client_fd`). The original `server_fd` remains open and keeps listening for other future clients!

### 5. `connect()`
* **What it is:** Initiates an active connection from a client socket to a remote server address.
* **Why use it:** Clients use this to kick off the TCP 3-way handshake (`SYN` -> `SYN-ACK` -> `ACK`).
* **Under the hood:** If the client hasn't called `bind()` beforehand, the kernel automatically chooses an ephemeral (random available) local port and assigns it to the client socket.

### 6. `send()` & `recv()`
* **What they are:** Transmits and receives raw bytes over a connected stream socket.
* **Why use it:** To exchange protocol payloads. Similar to standard `read()` / `write()`, but includes network flag modifiers (e.g. `MSG_DONTWAIT`, `MSG_PEEK`).
* **Under the hood:** TCP is a continuous byte stream without message boundaries. `send()` copies data from your application memory into the kernel's send buffer. `recv()` copies available bytes from the kernel's receive buffer to your program's buffer.

### 7. `close()`
* **What it is:** Closes the socket file descriptor and releases network resources.
* **Why use it:** Prevents resource leaks (file descriptors and memory) and cleanly initiates the TCP 4-way termination handshake (`FIN` / `ACK`).

---

## 📦 Level 1: Basic Single-Message TCP Communication

### Level 1 Overview
In Level 1, we implement the most fundamental building block: a synchronous, blocking TCP server and client.
- The server binds to port `8080`, listens for 1 incoming client, receives one message, replies with `"Hello back!"`, and shuts down.
- The client connects to `127.0.0.1:8080`, sends `"Hello"`, receives the reply, prints it, and exits.

Files:
- [`lev1/server.c`](file:///g:/cyber/socket/lev1/server.c)
- [`lev1/client.c`](file:///g:/cyber/socket/lev1/client.c)

---

### Level 1 Architecture & Sequence Wireframe

```
[ CLIENT ]                                             [ SERVER ]
    |                                                      |
socket() -> sockfd created                                 socket() -> server_fd created
    |                                                      |
    |                                                    bind() -> bound to 0.0.0.0:8080
    |                                                      |
    |                                                   listen() -> backlog queue: 5
    |                                                      |
    |                                                   accept() -> blocks & waits...
    |                                                      |
connect() ----------------[ TCP SYN ]--------------------> |
    | <-----------------[ TCP SYN + ACK ]----------------- |
    | -------------------[ TCP ACK ]---------------------> |
    |                                                   accept() wakes up!
    |                                                   Returns NEW client_fd
    |                                                      |
send("Hello") --------[ TCP PSH + ACK ]------------------> recv(buffer)
    |                                                   Prints "Client said: Hello"
    |                                                      |
recv(buffer) <-------[ TCP PSH + ACK ]------------------ send("Hello back!")
Prints "Server said: Hello back!"                          |
    |                                                   close(client_fd)
close(sockfd)                                           close(server_fd)
```

---

### Server Lifecycle Walkthrough
Refer to [`lev1/server.c`](file:///g:/cyber/socket/lev1/server.c):

1. **Endpoint Creation:**
   ```c
   server_fd = socket(AF_INET, SOCK_STREAM, 0);
   ```
   - `AF_INET`: Address Family for IPv4.
   - `SOCK_STREAM`: Reliable, sequenced 2-way byte stream (TCP).
   - `0`: Use the default protocol for this socket family (IPPROTO_TCP).

2. **Address Setup & Binding:**
   ```c
   struct sockaddr_in address;
   address.sin_family = AF_INET;
   address.sin_port = htons(8080);
   address.sin_addr.s_addr = INADDR_ANY;

   bind(server_fd, (struct sockaddr *)&address, sizeof(address));
   ```
   - `htons(8080)`: Converts integer `8080` from Host Byte Order (Little Endian on x86) to Network Byte Order (Big Endian).
   - `INADDR_ANY`: Binds to all available network interfaces (`0.0.0.0`), allowing connections across `localhost` and local LAN IPs.

3. **Listening & Accepting Connections:**
   ```c
   listen(server_fd, 5);
   client_fd = accept(server_fd, NULL, NULL);
   ```
   - Execution pauses on `accept()` until an incoming client completes the 3-way handshake.
   - `client_fd` is returned for data transfer. `server_fd` remains the welcoming listener.

4. **Receiving, Replying, and Teardown:**
   ```c
   int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
   buffer[n] = '\0'; // Null-terminate received string safely
   send(client_fd, "Hello back!", 11, 0);
   close(client_fd);
   close(server_fd);
   ```

---

### Client Lifecycle Walkthrough
Refer to [`lev1/client.c`](file:///g:/cyber/socket/lev1/client.c):

1. **Creating the socket:**
   ```c
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   ```

2. **Specifying target destination & IP conversion:**
   ```c
   server_address.sin_family = AF_INET;
   server_address.sin_port = htons(8080);
   inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
   ```
   - `inet_pton` (*Presentation to Numeric*): Converts the human-readable string `"127.0.0.1"` into raw binary network bytes (`0x7F000001`).

3. **Connecting and Exchanging Data:**
   ```c
   connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
   send(sockfd, "Hello", 5, 0);
   int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
   buffer[n] = '\0';
   printf("Server said: %s\n", buffer);
   close(sockfd);
   ```

---

### Crucial Concepts & Data Structures

| Concept | Explanation |
| :--- | :--- |
| `struct sockaddr_in` | IPv4 address structure containing the family (`AF_INET`), port (`sin_port`), and 32-bit IPv4 address (`sin_addr`). |
| `struct sockaddr` | Generic legacy socket address struct. Specific address structures (like `sockaddr_in` or `sockaddr_in6`) are cast to `(struct sockaddr *)` when passed into POSIX APIs. |
| `htons()` | **H**ost **TO** **N**etwork **S**hort. Translates integer byte order to standard network Big-Endian format. |
| `inet_pton()` | Modern standard function converting text IP addresses to binary network byte representation. Replaces obsolete `inet_addr()`. |
| Buffer Null-Termination | TCP sockets exchange raw byte streams, **not** C-strings. You must always manually place a null terminator `\0` at index `n` before using `%s` in `printf()`. |

---

### How to Compile and Run

#### 1. Compile both source files
```bash
gcc -Wall -Wextra -O2 lev1/server.c -o lev1/server
gcc -Wall -Wextra -O2 lev1/client.c -o lev1/client
```

#### 2. Open Terminal 1 and start the Server
```bash
./lev1/server
# Output:
# Server listening on port 8080...
```

#### 3. Open Terminal 2 and run the Client
```bash
./lev1/client
# Output:
# Server said: Hello back!
```

#### 4. Result in Terminal 1 (Server):
```
Client connected!
Client said: Hello
```

---

## 📦 Level 2: Continuous Bi-Directional Echo Session

### Level 2 Overview
In Level 2, we graduate from a single-shot `"Hello"` message to a **persistent, interactive session**. The client continuously accepts user input from standard input, sends it over the wire, and the server continuously echoes the exact data back until either peer disconnects.

Level 2 introduces critical production fundamentals:
1. **The Event Loop (`while (1)`)**: Keeps the connection alive for multiple exchanges.
2. **Defensive Error Handling**: Every socket system call checks for `< 0` and inspects `perror()`.
3. **TCP Connection Teardown Detection (`recv() == 0`)**: Detecting when the client closes the connection via EOF or termination, allowing clean resource reclamation without crashing.

Files:
- [`lev2/server.c`](file:///g:/cyber/socket/lev2/server.c)
- [`lev2/client.c`](file:///g:/cyber/socket/lev2/client.c)

---

### Level 2 Architecture & Sequence Wireframe

```
[ CLIENT (Terminal 2) ]                                     [ SERVER (Terminal 1) ]
         |                                                            |
     socket()                                                      socket()
         |                                                            |
         |                                                          bind() -> :8080
         |                                                            |
         |                                                         listen()
         |                                                            |
     connect() ==============[ TCP 3-Way Handshake ]===============> accept() -> client_fd
         |                                                            |
=============================== CONTINUOUS LOOP ===============================
         |                                                            |
  fgets(stdin)                                                        |
  send("ping\n", 5B) ----------------- [ TCP PSH ] -----------------> recv() -> returned 5B
         |                                                            send("ping\n", 5B)
  recv() -> returned 5B <------------- [ TCP PSH ] ------------------/
  printf("Server: ping")                                              |
         |                                                            |
  fgets(stdin)                                                        |
  send("how are you?\n") ------------ [ TCP PSH ] ------------------> recv() -> returned 13B
         |                                                            send("how are you?\n", 13B)
  recv() <---------------------------- [ TCP PSH ] ------------------/
===============================================================================
         |                                                            |
   User presses Ctrl+D                                                |
   (fgets returns NULL)                                               |
         |                                                            |
    close(sockfd)                                                     |
         | ---------------------- [ TCP FIN ] ----------------------> |
         |                                                         recv() returns 0!
         |                                                         ("Client disconnected.")
         |                                                            |
         |                                                         close(client_fd)
       exit                                                        close(server_fd)
```

---

### Server Lifecycle & Key Improvements Walkthrough
Refer to [`lev2/server.c`](file:///g:/cyber/socket/lev2/server.c):

1. **Defensive System Call Validation:**
   Every network operation can fail (port already bound, out of file descriptors, invalid permissions). Level 2 guards each call:
   ```c
   server_fd = socket(AF_INET, SOCK_STREAM, 0);
   if (server_fd < 0) { perror("socket"); return 1; }

   if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
       perror("bind"); return 1;
   }
   ```

2. **The Connection Lifecycle Loop:**
   Once `accept()` returns `client_fd`, the server enters an infinite loop waiting for frames:
   ```c
   while (1) {
       int n = recv(client_fd, buffer, sizeof(buffer), 0);
       ...
   }
   ```

3. **Mastering `recv()` Return Codes:**
   The heart of network I/O state management:
   ```c
   if (n == 0) {
       printf("Client disconnected.\n"); // Orderly TCP FIN received
       break;
   }
   if (n < 0) {
       perror("recv");                   // Socket read error / reset
       break;
   }
   ```

4. **Symmetric Echo:**
   The server echoes back the exact number of bytes it received without altering or assuming null-terminated strings:
   ```c
   int sent = send(client_fd, buffer, n, 0);
   ```

---

### Client Lifecycle & Key Improvements Walkthrough
Refer to [`lev2/client.c`](file:///g:/cyber/socket/lev2/client.c):

1. **Interactive Prompt with `fgets`:**
   Reads user input directly from the terminal. If the user signals End-Of-File (`Ctrl+D` on Linux/macOS or `Ctrl+Z` on Windows), `fgets` returns `NULL`, triggering a clean break:
   ```c
   while (1) {
       printf("You: ");
       if (fgets(buffer, sizeof(buffer), stdin) == NULL)
           break;
       ...
   }
   ```

2. **Dynamic Transmission:**
   Measures string length from the keyboard buffer and transmits it:
   ```c
   int length = strlen(buffer);
   int sent = send(sockfd, buffer, length, 0);
   ```

3. **Safe String Null-Termination:**
   Because TCP transmits raw byte streams without trailing null characters, the client explicitly terminates index `n` before printing:
   ```c
   int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
   if (n == 0) {
       printf("Server disconnected.\n");
       break;
   }
   buffer[n] = '\0';
   printf("Server: %s", buffer);
   ```

---

### Crucial Concepts & Patterns in Level 2

| Concept | Explanation |
| :--- | :--- |
| **`recv() == 0` (EOF)** | Signals that the remote peer initiated an orderly shutdown (sent a TCP `FIN`). It is **not** an error; it is the standard way to detect a closed connection. |
| **`recv() < 0`** | Indicates an abnormal error occurred (e.g., connection reset `ECONNRESET`, interrupted call `EINTR`). |
| **`perror()` & `errno`** | Prints a human-readable description of the last error code recorded in the global thread-local `errno` variable by the OS. |
| **Byte Count Integrity** | Notice `server.c` calls `send(client_fd, buffer, n, 0)` using the return value `n` from `recv()`. Never assume fixed sizes over a dynamic TCP stream. |

---

### How to Compile and Run

#### 1. Compile both source files
```bash
gcc -Wall -Wextra -O2 lev2/server.c -o lev2/server
gcc -Wall -Wextra -O2 lev2/client.c -o lev2/client
```

#### 2. Start the Server in Terminal 1
```bash
./lev2/server
# Output:
# Server listening on port 8080...
```

#### 3. Start the Client in Terminal 2 & Chat Interactively
```bash
./lev2/client
# Output:
# Connected to server!
# You: Hello Level 2!
# Sent 15 bytes
# Server: Hello Level 2!
# You: Socket programming in C is awesome.
# Sent 35 bytes
# Server: Socket programming in C is awesome.
```

#### 4. Disconnecting
Press `Ctrl+D` in Terminal 2 (or `Ctrl+C`).
- **Client Output:** Exits cleanly.
- **Server Output:**
  ```
  Client disconnected.
  ```

---

## 📦 Level 3: TCP Framing, Buffer Management & Command Parsing

### Level 3 Overview
A critical reality of TCP: **TCP is a byte-stream protocol, not a message protocol**. It guarantees in-order byte delivery, but has zero concept of application-level message boundaries.
- **Packet Fragmentation:** A single command might arrive in two separate `recv()` calls.
- **Packet Concatenation (Coalescing):** Multiple commands sent quickly might arrive combined inside a single `recv()` call.

Level 3 solves this by implementing **delimiter-based framing (`\n`)**, persistent buffer accumulation (`buffer_used`), and dynamic memory shifting (`memmove`) to parse protocol commands (`PING` -> `PONG\n`).

Files:
- [`lev3/server.c`](file:///g:/cyber/socket/lev3/server.c)
- [`lev3/client.c`](file:///g:/cyber/socket/lev3/client.c)

---

### Level 3 Architecture & Buffer Wireframe

```
Raw TCP Stream from Network:
+-------------------------------------------------------------+
| "PING\nPING\nINCOMP" ... (next packet delivers "LETE\n")   |
+-------------------------------------------------------------+

Accumulation Buffer [4096 bytes]:
Step 1: recv() appends data at (buffer + buffer_used)
+------------------------------------+------------------------+
| P I N G \n P I N G \n I N C O M P  |       Empty Space      |
+------------------------------------+------------------------+
 ^          ^
 |          +--- strchr('\n') finds first message boundary
 |
 Dispatches command "PING" -> sends "PONG\n"

Step 2: memmove() shifts remaining unprocessed bytes to the front:
+--------------------------+----------------------------------+
| P I N G \n I N C O M P   |            Empty Space           |
+--------------------------+----------------------------------+
             ^
             +--- Loops again: finds second message boundary!
```

---

### Server Lifecycle & Buffer Walkthrough
Refer to [`lev3/server.c`](file:///g:/cyber/socket/lev3/server.c):

1. **Incremental Buffer Offset:**
   The server reads directly into the unused slice of the buffer:
   ```c
   int n = recv(client_fd, buffer + buffer_used, BUFFER_SIZE - buffer_used - 1, 0);
   buffer_used += n;
   buffer[buffer_used] = '\0';
   ```

2. **Scanning for Delimiters:**
   Using `strchr(buffer, '\n')`, the server searches for complete command terminations:
   ```c
   while ((newline = strchr(buffer, '\n')) != NULL) {
       *newline = '\0'; // Temporarily terminate token

       if (strcmp(buffer, "PING") == 0)
           send(client_fd, "PONG\n", 5, 0);
       else
           send(client_fd, "NOT_IMPLEMENTED\n", 16, 0);
   ```

3. **Buffer Compaction with `memmove`:**
   After handling a command, the consumed bytes must be evicted, and leftover bytes shifted to index `0`:
   ```c
       int consumed = (newline - buffer) + 1;
       memmove(buffer, buffer + consumed, buffer_used - consumed);
       buffer_used -= consumed;
       buffer[buffer_used] = '\0';
   }
   ```

---

### Crucial Concepts in Level 3

| Concept | Explanation |
| :--- | :--- |
| **`memmove` vs `memcpy`** | `memmove` safely handles **overlapping memory regions**, which is guaranteed to occur when shifting bytes inside the same buffer array. `memcpy` on overlapping memory causes undefined behavior. |
| **Frame Boundaries** | Delimiter framing (`\n`, `\r\n`) is simple and human-readable (used in HTTP/1.1, Redis RESP, SMTP). Binary protocols often use **length-prefixed framing** (e.g. 4-byte header specifying payload size). |

---

### How to Compile and Run

```bash
# Terminal 1:
gcc -Wall -Wextra -O2 lev3/server.c -o lev3/server
./lev3/server

# Terminal 2:
gcc -Wall -Wextra -O2 lev3/client.c -o lev3/client
./lev3/client
# > PING
# Server: PONG
# > TEST
# Server: NOT_IMPLEMENTED
```

---

## 📦 Level 4: Multi-Process Concurrent Server with `fork()`

### Level 4 Overview
In Levels 1 through 3, the server was **single-client blocking**: while interacting with Client A, the server was stuck in `recv()` and could not `accept()` any connection from Client B.

Level 4 introduces **process-level concurrency** using the POSIX `fork()` system call:
- The **Parent Process** dedicates 100% of its time to accepting incoming connections in an infinite loop.
- Upon each connection, it spawns an isolated **Child Process** (`fork()`) dedicated solely to serving that client.

Files:
- [`lev4/server.c`](file:///g:/cyber/socket/lev4/server.c)

---

### Level 4 Architecture & Process Wireframe

```
                     +---------------------------------+
                     |   PARENT PROCESS (pid = 1000)   |
                     |       Listens on server_fd      |
                     +----------------+----------------+
                                      |
                      accept() returns client_fd (e.g. fd=4)
                                      |
                               fork() called
                               /             \
                             /                 \
    +-----------------------+                   +-----------------------+
    |     PARENT PROCESS    |                   |     CHILD PROCESS     |
    |      (pid = 1000)     |                   |      (pid = 1001)     |
    +-----------------------+                   +-----------------------+
    | close(client_fd)      |                   | close(server_fd)      |
    | Does NOT need fd=4    |                   | Does NOT need listener|
    | Loops back to accept()|                   | handle_client(fd=4)   |
    +-----------------------+                   | exit(0) when finished |
                                                +-----------------------+
```

---

### Key Implementations & Mechanics
Refer to [`lev4/server.c`](file:///g:/cyber/socket/lev4/server.c):

1. **The Concurrency Fork Loop:**
   ```c
   while (1) {
       client_fd = accept(server_fd, NULL, NULL);
       pid_t pid = fork();

       if (pid == 0) {
           // CHILD PROCESS:
           close(server_fd);          // Child doesn't need to listen
           handle_client(client_fd);
           exit(0);                   // Child terminates upon client disconnect
       }

       // PARENT PROCESS:
       close(client_fd);              // Parent closes duplicate client socket
   }
   ```

2. **File Descriptor Reference Counting:**
   Under Unix, when `fork()` runs, the child receives a duplicate copy of the parent's file descriptor table pointing to the same underlying kernel file objects.
   - If the parent does **not** call `close(client_fd)`, the kernel ref count stays $\ge 1$, and TCP `FIN` will **never** be sent to the client when the child terminates!
   - Both parent and child must explicitly close the descriptors they do not use.

---

### How to Compile and Run

```bash
gcc -Wall -Wextra -O2 lev4/server.c -o lev4/server
./lev4/server
```
Test with multiple concurrent client terminals:
```bash
# Terminal 2 (Client 1):
nc 127.0.0.1 8080

# Terminal 3 (Client 2 - runs simultaneously without waiting!):
nc 127.0.0.1 8080
```

---

## 📦 Level 5: Thread-Safe In-Memory Key-Value Store (`pthreads`)

### Level 5 Overview
While `fork()` provides memory isolation, multi-client servers often require **shared state** (e.g., an in-memory database like Redis or Memcached).

Level 5 designs a custom in-memory Hash Table with bucket chaining, made completely **thread-safe** across concurrent POSIX threads using mutex locks (`pthread_mutex_t`).

Files:
- [`lev5/server.c`](file:///g:/cyber/socket/lev5/server.c)

---

### Level 5 Architecture & Hash Table Wireframe

```
 HashTable Struct
+-----------------------------------------------------------------+
| pthread_mutex_t lock  <--- Guards all concurrent reads/writes   |
| buckets[16]                                                     |
+-----------------------------------------------------------------+
    [0] -> NULL
    [1] -> [ "name": "Priyanshu" ] -> NULL
    [2] -> [ "role": "admin" ] -> [ "env": "prod" ] -> NULL (Chaining)
    ...
    [15]-> NULL
```

---

### Core Data Structures & Concurrency Mechanics
Refer to [`lev5/server.c`](file:///g:/cyber/socket/lev5/server.c):

1. **djb2 Hashing Algorithm:**
   A fast, highly distributed bit-shifting string hash:
   ```c
   unsigned int hash(const char *key) {
       unsigned int hash = 5381;
       while (*key) {
           hash = ((hash << 5) + hash) + *key; // hash * 33 + c
           key++;
       }
       return hash % TABLE_SIZE;
   }
   ```

2. **Critical Section Synchronization:**
   Every mutating and lookup operation acquires the mutex before traversing buckets and releases it upon completion:
   ```c
   pthread_mutex_lock(&table->lock);
   // ... safely manipulate buckets, insert / retrieve / delete nodes ...
   pthread_mutex_unlock(&table->lock);
   ```

3. **Multi-Threaded Spawning:**
   Three concurrent threads write and read simultaneously without race conditions or memory corruption:
   ```c
   pthread_t thread1, thread2, thread3;
   pthread_create(&thread1, NULL, worker, &table);
   pthread_create(&thread2, NULL, worker, &table);
   pthread_create(&thread3, NULL, worker, &table);

   pthread_join(thread1, NULL);
   pthread_join(thread2, NULL);
   pthread_join(thread3, NULL);
   ```

---

### How to Compile and Run

```bash
# Must link with -pthread
gcc -Wall -Wextra -O2 lev5/server.c -o lev5/server -pthread
./lev5/server
```

---

## 📦 Level 6: High-Performance Event Loop with Linux `epoll`

### Level 6 Overview
The **C10K problem**: Spawning 10,000 processes (`fork`) or 10,000 threads consumes gigabytes of stack memory and destroys CPU throughput via context-switching overhead.

Level 6 implements the modern industry standard for high-performance network engines (used by **Nginx, Node.js, and Redis**): **I/O Multiplexing with Linux `epoll`**.
A single thread monitors hundreds or thousands of non-blocking sockets simultaneously via OS kernel-driven readiness notifications in $O(1)$ time complexity.

Files:
- [`lev6/server.c`](file:///g:/cyber/socket/lev6/server.c)

---

### Level 6 Architecture & Event Loop Wireframe

```
+-------------------------------------------------------------------------+
|                              Linux Kernel                               |
|                                                                         |
|  [epoll Interest List]              [epoll Ready List]                  |
|  - server_fd (8080)                 Events available now:               |
|  - client_fd 4 (idle)               1. server_fd (New incoming client)  |
|  - client_fd 5 (ready to read!) ==> 2. client_fd 5 (50 bytes ready)     |
+--------------------------------------------|----------------------------+
                                             |
                  epoll_wait() awakes with ready events
                                             v
+-------------------------------------------------------------------------+
|                  Single-Threaded Application Event Loop                 |
|                                                                         |
|  for each event:                                                        |
|    if (fd == server_fd)  --> Non-blocking accept() until EAGAIN         |
|    if (fd == client_fd)  --> Non-blocking recv() & immediate send()     |
+-------------------------------------------------------------------------+
```

---

### Core Mechanics & API Walkthrough
Refer to [`lev6/server.c`](file:///g:/cyber/socket/lev6/server.c):

1. **Non-Blocking Sockets via `fcntl`:**
   By default, socket operations block the thread indefinitely. `O_NONBLOCK` ensures system calls return immediately with `EAGAIN` / `EWOULDBLOCK` if no data is pending:
   ```c
   int make_nonblocking(int fd) {
       int flags = fcntl(fd, F_GETFL, 0);
       return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
   }
   ```

2. **Epoll Setup (`epoll_create1` & `epoll_ctl`):**
   ```c
   epoll_fd = epoll_create1(0);

   event.events = EPOLLIN;
   event.data.fd = server_fd;
   epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
   ```

3. **Event Loop (`epoll_wait`):**
   Sleeps efficiently in kernel space until one or more monitored sockets have I/O readiness:
   ```c
   while (1) {
       int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

       for (int i = 0; i < n_events; i++) {
           int fd = events[i].data.fd;

           if (fd == server_fd) {
               // Drain all pending incoming connections
               while (1) {
                   int client_fd = accept(server_fd, NULL, NULL);
                   if (client_fd < 0) break; // EAGAIN
                   make_nonblocking(client_fd);
                   epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
               }
           } else {
               // Client ready to read
               int bytes = recv(fd, buffer, sizeof(buffer), 0);
               if (bytes == 0) {
                   epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                   close(fd);
               } else {
                   send(fd, buffer, bytes, 0);
               }
           }
       }
   }
   ```

---

### Comparative Architecture Matrix

| Metric / Mechanism | Level 1 & 2 (Iterative) | Level 4 (`fork`) | Level 5 (`pthread`) | Level 6 (`epoll`) |
| :--- | :--- | :--- | :--- | :--- |
| **Concurrency Model** | Single client only | Process per client | Thread per client / pool | Event multiplexing |
| **Memory Footprint** | Minimal | High (process image copy) | Medium (thread stack ~2-8MB) | Lowest (single stack) |
| **Context Switching** | None | High (OS process switch) | Medium (kernel thread switch) | Zero (single-threaded) |
| **Shared State** | Trivial | Difficult (Shared Memory/IPC) | Easy (Mutex-protected structs) | Trivial (same memory space) |
| **Connection Capacity** | 1 | 100s | 1,000s | 100,000+ (C10K / C1000K) |

---

### How to Compile and Run

```bash
# Note: Linux kernel / WSL / Linux environment required for <sys/epoll.h>
gcc -Wall -Wextra -O2 lev6/server.c -o lev6/server
./lev6/server
```

---

*Authored during the Socket Programming in C mastery journey.*
