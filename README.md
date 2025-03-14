# Client-Server Application for File Transfer (Version 1)

## Overview
This project implements a **Client-Server application** over **TCP** that allows clients to perform various file operations on the server, including **uploading, downloading, listing, deleting**, and **retrieving file information**. The communication follows a **custom protocol** for handling these requests and responses.

The system includes a **Socket class** for handling low-level socket operations, a **Server class** for managing connections and file operations, and a **Client class** for interacting with the server. A **ClientCLI class** provides a simple command-line interface for users to interact with the server.

## Features
- **File Operations**: Implemented commands to handle files: **GET**, **PUT**, **LIST**, **DELETE**, and **INFO**.
- **Client-Server Communication**: TCP-based communication with a custom protocol for sending and receiving data.
- **Message Prefix**: A 4-byte length prefix indicating the size of the data being sent, ensuring reliable data transfer.
- **Error Handling**: Proper error codes for invalid operations (e.g., file not found, permission errors, server failures).
- **ClientCLI**: A command-line interface to interact with the server and perform file operations.
- **UTF-8 Encoding**: All text data (e.g., filenames, messages) is encoded in **UTF-8**, where **1 character = 1 byte**.

## Installation and Compilation

### **1. Clone the Repository**
```bash
git clone https://github.com/iravelmakina/client-server.git
cd client-server
```

### **2. Compile the Program**
#### **Using g++ (Linux/macOS)**
The program requires **C++11 or higher**.
```bash
g++ -std=c++11 -pthread socket/Socket.cpp -o socket/Socket.o
ar rcs libsocket.a socket/Socket.o
g++ -std=c++11 -pthread server/Server.cpp server/main.cpp -L. -lsocket -o server
g++ -std=c++11 -pthread client/Client.cpp client/main.cpp client/ClientCLI.cpp -L. -lsocket -o client
```

#### **Using CMake**
Alternatively, use CMake for a more structured build process.

### Summary of Compilation Commands:

1. **Compile Socket Library**:

   Navigate to the `socket/src` directory and compile the `Socket.cpp` file, then create the static library:

   ```bash
   cd socket/src
   g++ -c Socket.cpp -o Socket.o
   ar rcs libsocket.a Socket.o
   ```

2. **Compile Server**:

   Navigate to the `server/src` directory, compile the `Server.cpp` and `main.cpp` files, and link them with the static library `libsocket.a`:

   ```bash
   cd server/src
   g++ -c Server.cpp -o Server.o
   g++ -c main.cpp -o main.o
   g++ Server.o main.o -L../../socket/src -lsocket -o server
   ```

3. **Compile Client**:

   Navigate to the `client/src` directory, compile the `Client.cpp`, `ClientCLI.cpp`, and `main.cpp` files, and link them with the static library `libsocket.a`:

   ```bash
   cd client/src
   g++ -c Client.cpp -o Client.o
   g++ -c ClientCLI.cpp -o ClientCLI.o
   g++ -c main.cpp -o main.o
   g++ Client.o ClientCLI.o main.o -L../../socket/src -lsocket -o client
   ```

## Usage

### **Commands Supported**:
- **LIST**: List all files in the server's directory.
- **GET <filename>**: Download a file from the server.
- **PUT <filename>**: Upload a file to the server.
- **DELETE <filename>**: Delete a file from the server.
- **INFO <filename>**: Retrieve metadata (size, last modified, etc.) of a file on the server.
- **EXIT**: Disconnect from the server.

### **Example**:
- To **list files**:
  ```bash
  LIST
  ```
- To **download a file**:
  ```bash
  GET myfile.txt
  ```
- To **upload a file**:
  ```bash
  PUT myfile.txt
  ```
- To **delete a file**:
  ```bash
  DELETE myfile.txt
  ```
- To **get file info**:
  ```bash
  INFO myfile.txt
  ```

## Code Structure
```
client-server/
│── socket/                   # Contains all socket-related code
│   ├── include/              # Header files for socket library
│   │   └── Socket.h          # Header file for the Socket class
│   ├── src/                  # Source files for socket library
│   │   └── Socket.cpp        # Implementation of the Socket class
│   ├── CMakeLists.txt        # CMake configuration for socket library
│
│── server/                   # Contains the server-side code
│   ├── files/                # Folder containing files for transfer or operations
│   ├── include/              # Header files for server-side code
│   │   └── Server.h          # Header file for the Server class
│   ├── src/                  # Source files for server-side code
│   │   ├── Server.cpp        # Implementation of the Server class
│   │   └── main.cpp          # Entry point for the server application
│   ├── CMakeLists.txt        # CMake configuration for server
│
│── client/                   # Contains the client-side code
│   ├── files/                # Folder containing files for transfer or operations
│   ├── include/              # Header files for client-side code
│   │   ├── Client.h          # Header file for the Client class
│   │   └── ClientCLI.h       # Header file for the ClientCLI class
│   ├── src/                  # Source files for client-side code
│   │   ├── Client.cpp        # Implementation of the Client class
│   │   ├── ClientCLI.cpp     # Implementation of the ClientCLI class
│   │   └── main.cpp          # Entry point for the client application
│   ├── CMakeLists.txt        # CMake configuration for client
│
│── CMakeLists.txt            # Root CMake configuration file for the whole project
│── .gitignore                # Git ignore file for excluding unnecessary files
│── .gitattributes            # Git attributes file
│── README.md                 # Project documentation (this file)

```

## Protocol Description (Version 1)

### **Message Format**:
- **Prefix**: 4-byte unsigned integer (network byte order) indicating the length of the data.
- **Data**: The actual message or file data, length specified by the prefix. All text data is encoded in **UTF-8**, where **1 character = 1 byte**.

### **Communication Example**:

| **Action**            | **Client**                          | **Message Length**                                              | **Server**                                                                                                                                   | **Message Length**                                              |
|-----------------------|-------------------------------------|-----------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------|
| **Connect**           | –                                   | –                                                               | `200 OK`                                                                                                                                     | 6 bytes                                                         |
| **Version Handshake** | `<version>` ex. `1.0.`              | Variable length max 512: version length                         | `200 OK` / `400 BAD REQUEST`: Invalid version.                                                                                               | Variable length max 512: (6/34)                                                         |
| **LIST**              | `LIST`                              | 4 bytes                                                         | List of files / `500 SERVER ERROR`: Failed to open directory. / `204 NO CONTENT`: The directory is empty.                                    | Variable length max 512: response length (43/39)                |
| **GET**               | `GET <filename>`                    | Variable length max 512: 4 + filename length                    | `200 OK` / `400 BAD REQUEST`: Invalid filename. / `404 NOT FOUND`: File does not exist.                                                      | Variable length max 512: (6/34/35)                              |
|                       | `ACK`                               | 3                                                               | File data + `""` (EOF)                                                                                                                       | Variable length, chunks of 1024 bytes followed by empty string. |
| **PUT**               | `PUT <filename>`                    | Variable length max 512: 4 + filename length                    | `200 OK` / `400 BAD REQUEST`: Invalid filename. / `500 SERVER ERROR`: Unable to create file.                                                 | Variable length max 512: (6/34/40)                              |
|                       | File data + `""` (EOF)              | Variable length, chunks of 1024 bytes followed by empty string. | `200 OK`                                                                                                                                     | 6                                                               |
| **INFO**              | `INFO <filename>`                   | Variable length max 512: 5 + filename length                    | File info / `400 BAD REQUEST`: Invalid filename. / `500 SERVER ERROR`: Unable to retrieve file info. / `404 NOT FOUND`: File does not exist. | Variable length max 512: response length (34/47/35)             |
| **DELETE**            | `DELETE <filename>`                 | Variable length max 512: 7 + filename length                    | `200 OK` / `400 BAD REQUEST`: Invalid filename. / `500 SERVER ERROR`: Unable to delete file. / `404 NOT FOUND`: File does not exist.         | Variable length max 512: (6/34/40/35)                           |
| **EXIT**              | `EXIT`                              | 4                                                               | –                                                                                                                                            | –                                                               |

## License

This project is open-source under the **MIT License**.
