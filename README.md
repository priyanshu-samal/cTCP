# 🌐 Socket Programming in C — The Journey from Scratch

> A structured, hands-on roadmap to mastering low-level network programming with Berkeley Sockets in C. Designed as both a personal learning log and a standalone reference guide.

---

## 📑 Table of Contents

- [🧠 The Core Mental Model](#-the-core-mental-model)
- [🔌 Fundamental Socket API Breakdown](#-fundamental-socket-api-breakdown)
- [🗺️ Journey Roadmap & Index](#️-journey-roadmap--index)
- [📦 Level 1: Basic Single-Message TCP Communication](#-level-1-basic-single-message-tcp-communication)
  - [Overview](#level-1-overview)
  - [Architecture & Sequence Wireframe](#level-1-architecture--sequence-wireframe)
  - [Server Lifecycle Walkthrough](#server-lifecycle-walkthrough)
  - [Client Lifecycle Walkthrough](#client-lifecycle-walkthrough)
  - [Crucial Concepts & Data Structures](#crucial-concepts--data-structures)
  - [How to Compile and Run](#how-to-compile-and-run)
- [🧭 Next Levels (Upcoming)](#-next-levels-upcoming)

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

## 🗺️ Journey Roadmap & Index

| Level | Directory | Topic / Focus | Status |
| :---: | :---: | :--- | :---: |
| **01** | [`lev1`](file:///g:/cyber/socket/lev1) | Basic Single-Message TCP Server & Client | Completed |
| **02** | `lev2` | Continuous Communication & Dynamic Loops | Coming Soon |
| **03** | `lev3` | Multi-Client Handling via Forking / Threads | Planned |
| **04** | `lev4` | Non-blocking I/O & Multiplexing (`select` / `poll`) | Planned |
| **05** | `lev5` | Modern High-Performance Event Loops (`epoll` / `kqueue`) | Planned |

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

## 🧭 Next Levels (Upcoming)

When you are ready to expand the journey:
- **Level 2:** Continuous bi-directional conversation loop (keep communicating until `exit` is typed).
- **Level 3:** Multi-client concurrent server using `fork()` or POSIX threads (`pthreads`).
- **Level 4:** Multiplexing I/O with `poll()` or `select()` (handling multiple connections in a single thread without blocking).
- **Level 5:** Scalable event notification with Linux `epoll`.

---

*Authored during the Socket Programming in C mastery journey.*
