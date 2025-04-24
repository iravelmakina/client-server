# Client-Server Architecture Implementation (v1 & v2)

## Overview
This repository contains two versions of the server:

- **v1**: The original implementation, designed for basic file operations and client-server communication.
- **v2**: An improved version with enhanced features, including support for multiple simultaneous clients, client authentication, and more.

The `main` branch contains the **latest stable version** of the server (currently **v2**), which is backward-compatible with **v1** clients.

---

## Features by Version

### **v1 Features**:
- Basic file operations: **GET**, **PUT**, **LIST**, **DELETE**, and **INFO**.
- Simple client-server communication with a custom protocol.
- No support for multiple simultaneous clients or client authentication.

### **v2 Features**:
- **Multiple Simultaneous Clients**: Support for handling multiple clients using a thread pool.
- **Client Authentication**: Clients must provide a valid username to connect.
- **Separate Folders for Clients**: Each client has a dedicated folder for file operations.
- **Backward Compatibility**: Supports **v1** clients with version detection.
- **Timeouts and Enhanced Message Receiving**: Improved handling of unresponsive clients.
- **Statistics Tracking**: Tracks and displays server command statistics.
- **Server CLI Stop Functionality**: Gracefully stop the server by pressing `q` in the server CLI.

---

## Compatibility

| Client Version | Compatible Server Versions          |
|----------------|-------------------------------------|
| **v1**         | ✅ v1, ✅ v2 (backward-compatible)  |
| **v2**         | ✅ v2                     |

---

## How to Use Different Versions

### **Switching Between Versions**
To use **v1**:
```bash
git checkout v1
```

To use **v2**:
```bash
git checkout v2
```

### **Running the Server**
Compile and run the server:
```bash
g++ -o server Server.cpp Socket.cpp -lpthread
./server
```

---

## Version-Specific Documentation
For detailed documentation on each version, refer to the `README.md` in the respective branch:

- **[v1 Documentation](https://github.com/iravelmakina/client-server/tree/version-1)**
- **[v2 Documentation](https://github.com/iravelmakina/client-server/tree/version-2)**

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

## Contributor

- [@iravelmakina](https://github.com/iravelmakina)
