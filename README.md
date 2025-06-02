# 🧠 mini\_serv – 42 Final Exam Practice

## 📋 Project Overview

`mini_serv` is a small network server written in C for the 42 school exam preparation.
The goal is to implement a **multi-client chat server** over TCP using only a restricted set of system calls and functions.

The server listens on `127.0.0.1` and allows clients to connect and broadcast messages to one another.

---

## 🎯 Assignment Summary

* Listens only on `127.0.0.1` at a port passed as argument.
* Uses `select()` to manage multiple clients without blocking.
* Assigns each client a unique **ID** (starting from 0).
* Broadcasts the following:

  * When a client joins:
    `server: client %d just arrived\n`
  * When a client leaves:
    `server: client %d just left\n`
  * When a client sends a message:
    Prefix each line with `client %d: ` and send to all other clients.

---

## ⚙️ Usage

### Compile:

```bash
gcc -Wall -Wextra -Werror -o mini_serv mini_serv.c
```

### Run:

```bash
./mini_serv [port]
```

Example:

```bash
./mini_serv 4242
```

### Test with `nc`:

In one terminal:

```bash
./mini_serv 4242
```

In others:

```bash
nc 127.0.0.1 4242
```

Type messages and observe how they're broadcasted.

---

## ❗ Constraints

* Only use the following allowed functions:
  `write, close, select, socket, accept, listen, send, recv, bind, strstr, malloc, realloc, free, calloc, bzero, atoi, sprintf, strlen, exit, strcpy, strcat, memset`
* **No** use of `SIGINT`, `SIGPIPE`, `poll`, `epoll`, etc.
* Must **not** use `#define`
* Must **not** disconnect clients who stop reading
* Must not leak file descriptors or memory

---

## 🧪 What I Learned

* How to use `select()` to handle multiple sockets
* How to implement basic TCP server logic from scratch
* Buffering incoming data to handle partial and multiline messages
* The importance of clean resource management (no leaks!)
* That **`SIGINT` handling is not compatible** with this project’s constraints — it was tested, but **could not be used in the final version** due to exam rules

---

## 🛑 Error Handling

* Missing arguments → `Wrong number of arguments\n` and exit(1)
* System call or memory failure → `Fatal error\n` and exit(1)

---

## 🛠️ Tip for Reviewers

To reset sockets cleanly during testing:

```bash
lsof -i :4242
kill -9 <PID>
```

---

## 📚 Notes

This project is purely for exam prep and learning purposes. It’s a great exercise in low-level networking, I/O multiplexing, and code discipline.
