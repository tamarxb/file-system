# Custom Inode-Based File System (IFS)

A specialized **low-level systems project** built in **C++** that simulates a Unix-style file system. This project manages data at the **sector level**, utilizing a custom virtual disk to handle file storage and metadata via an **Inode-based architecture**.

\---

## Project Overview

This project serves as a deep dive into how data is organized on physical storage. By bypassing the OS's native file system, this engine manages its own **virtual disk image**, handling the mapping between logical files and physical sectors.

### Key Features

* **Inode Metadata Management:** Custom structures to store file permissions, size, and data block pointers.
* **Sector-Based Storage:** Direct management of virtual disk sectors, ensuring data is written and read at specific byte offsets.
* **Path Resolution:** Logic to navigate and locate files within a directory structure.
* **Custom Interactive Shell:** A built-in CLI for real-time interaction with the file system.
* **Modular OOP Design:** Built with clear separation between the **Disk Driver**, **Inode Manager**, and the **Shell Interface**.

\---

## Technical Specifications

* **Language:** C++
* **Core Logic:** Inode-based indexing
* **Storage Medium:** Binary Virtual Disk (.bin / .img)
* **Development Status:** Stable release (v1.0) - April 2026

\---

## Configuration \& Scaling

The file system is designed with a **dynamic storage architecture**. You can easily scale the virtual disk size to fit your needs (up to **1GB**).

To change the disk size:

1. Open `blkdev.h`.
2. Locate the `DEVICE\_SIZE` constant.
3. Update the value to your desired capacity (in bytes).
4. Recompile the project using `make`.

\---

## Supported Commands

Interact with the file system using the following shell commands:

|Command|Description|
|-|-|
|`ls`|Lists all the files and directories in the current directory.|
|`cat`|Displays the actual content stored inside a specific file.|
|`touch`|Creates a new, empty file on the virtual disk.|
|`mkdir`|Creates a new directory (folder) within the file system.|
|`edit`|Allows you to write new data into an existing file.|
|`help`|Shows a list of all available commands and how to use them.|
|`exit`|Closes the file system shell.|

\---

## Getting Started

### Prerequisites

* **Build Tool:** `make` utility installed.
* A C++ compiler supporting **C++17** or higher



### Build and Run
1. **Clone the repository:**
   ```bash
   git clone [https://github.com/yosef926/FileSystem](https://github.com/tamarxb/file-system)
   cd file-system
2. **Compile the engine**
   ```bash
   make
3. **Launch the system**
   ```bash
   /bin/myfs <blkdevname>

---

## Future Roadmap
While the current version (v1.0) is a functional prototype, I plan to implement the following features to further simulate a production-grade file system:
* **File Deletion & Reclamation:** Implementing a mechanism to free up Inodes and sectors when files are removed.
* **Nested Directories:** Expanding the path resolution logic to support deeply nested folder structures.
* **File Permissions:** Adding Read/Write/Execute flags to the Inode metadata.
* **Multi-threading Support:** Implementing mutexes to handle concurrent access to the virtual disk.

## Learning Objectives
Through the development of this project, I gained hands-on experience in:
* **Binary Data Management:** Understanding how to manipulate raw bytes and offsets on a simulated disk.
* **OS Internals:** Deepening my knowledge of Inode tables, bitmasking for allocation, and sector management.
* **Low-Level C++:** Mastering pointer arithmetic, memory-safe data structures, and decoupled architecture design.

## Engineering Insights
The core of this project is the Sector Manager, which simulates raw disk I/O. One of the primary technical challenges was implementing the pointer logic within the Inodes to ensure that as a file grows, the system correctly identifies and links the next available sector on the virtual disk without data overlap.
