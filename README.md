# 📘 Chat Application Documentation

## 🚀 Project Overview
This is a dual-mode Chat Application developed in **C** for Linux/WSL environments. It demonstrates two fundamental Operating System concepts for Inter-Process Communication (IPC):
1.  **Shared Memory (Local Mode):** Communication between processes on the same machine using POSIX Shared Memory and Semaphores.
2.  **TCP Sockets (Network Mode):** Communication over a network using TCP/IP sockets.

The application features a graphical user interface (GUI) built with **GTK+ 3**.

---

## 📂 Project Structure

| File | Description |
|------|-------------|
| `main.c` | Entry point. Handles the main loop, login window, and switching between modes. Contains cleanup logic. |
| `gui.c` | Handles all GTK+ 3 graphical interface logic (Login window, Chat window, Buttons, Styling). |
| `gui.h` | Header file for GUI functions. |
| `shared_memory_client.c` | Implementation of the Shared Memory chat client. Uses `mmap` and `semaphores`. |
| `tcp_client.c` | Implementation of the TCP chat client. Uses `sockets` and `pthreads`. |
| `tcp_server.c` | The TCP Server implementation. Handles multiple clients and broadcasts messages. |
| `client.h` | Header file defining entry points for both client modes. |
| `common.h` | Common constants and data structures used across the project. |
| `Makefile` | Build script to compile the project and link dependencies. |

---

## 🛠️ How It Works

### 1. Shared Memory Mode (Local)
Designed for two local clients (Client 1 & Client 2).
*   **Mechanism:** Uses POSIX Shared Memory (`shm_open`, `mmap`) to create a shared memory region accessible by both processes.
*   **Synchronization:** Uses POSIX Semaphores (`sem_open`, `sem_wait`, `sem_post`) to coordinate reading and writing, preventing race conditions.
*   **Flow:**
    *   Client 1 writes to `c1_to_c2` buffer and signals Client 2.
    *   Client 2 waits for the signal, reads the message, and displays it.
    *   Vice versa for Client 2 to Client 1.

### 2. TCP Network Mode
Designed for communication over a network (or localhost).
*   **Mechanism:** Uses standard BSD Sockets (`socket`, `bind`, `listen`, `connect`).
*   **Architecture:** Client-Server model.
*   **Server (`tcp_server.c`):**
    *   Listens on port `8080`.
    *   Accepts incoming connections.
    *   Creates a separate thread for each client.
    *   Broadcasts received messages to all other connected clients.
*   **Client (`tcp_client.c`):**
    *   Connects to the server IP.
    *   Uses a background thread (`pthread`) to listen for incoming messages from the server.
    *   Sends messages to the server when the user clicks "Send".

---

## 🖥️ Graphical User Interface (GUI)
*   **Library:** GTK+ 3.
*   **Features:**
    *   **Login Window:** Allows selecting mode, entering name, and Client ID/IP.
    *   **Chat Window:** Displays message history and input field.
    *   **Auto-Cleanup:** Automatically kills the server and frees resources upon exit.
    *   **Styling:** Custom CSS for a modern look.

---

## 🚀 Getting Started (Step-by-Step from Scratch)

If you are on Windows, follow these steps to set up the environment and run the project.

### 1️⃣ Install Ubuntu (WSL)
1.  Open **Microsoft Store**.
2.  Search for **"Ubuntu"** (e.g., Ubuntu 22.04 LTS).
3.  Click **Get** or **Install**.
4.  Once installed, open **Ubuntu** from the Start menu.
5.  Wait for the installation to complete and create a **username** and **password** when prompted.

### 2️⃣ Install Required Tools
In the Ubuntu terminal, run the following commands to install the C compiler and GUI libraries:

```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-3-dev -y
```
*(Enter your password if asked. It won't show on screen while typing).*

### 3️⃣ Navigate to Project Folder
Assuming the project is on your Windows Desktop, navigate to it using:

```bash
cd /mnt/c/Users/Bishoy/Desktop/OS\ project
```
*(Note: Adjust the path if your username or folder name is different).*

### 4️⃣ Compile the Project
Run the `make` command to build the application:

```bash
make clean
make
```
This will create the executable files (`chat_app` and `tcp_server`).

### 5️⃣ Run the Application
Start the chat application:

```bash
./chat_app
```

### 6️⃣ Usage Scenarios

#### Scenario A: Shared Memory (2 Terminals)
1.  **Terminal 1:** Run `./chat_app` -> Select **Shared Memory** -> Name: "Alice", ID: **1**.
2.  **Terminal 2:** Run `./chat_app` -> Select **Shared Memory** -> Name: "Bob", ID: **2**.
3.  Chat!

#### Scenario B: TCP Network
1.  Run `./chat_app`.
2.  Select **TCP Network**.
3.  Click **Start Local Server** (or ensure `tcp_server` is running elsewhere).
4.  Enter Name and IP (default `127.0.0.1` for local).
5.  Click **Connect**.

---

## 🧹 Resource Management
The application ensures proper cleanup:
*   **Shared Memory:** Unmaps memory and closes semaphores on exit.
*   **TCP:** Closes sockets and joins threads.
*   **Server:** If started via GUI, the server process is automatically killed when the app closes.

---

## 👨‍💻 Author
**Bishoy** - Operating Systems Project
