# Campus Network Communication System

A client-server application that simulates inter-campus communication for a multi-campus university network using TCP and UDP protocols.

## 👥 Team Members

- **23F-0734**
- **23F-0839**
- **23F-0807**

**Section:** BSCS 5B

---

## 📋 Table of Contents

- [Project Overview](#project-overview)
- [Features](#features)
- [Architecture](#architecture)
- [Technologies Used](#technologies-used)
- [Project Structure](#project-structure)
- [Installation](#installation)
- [Usage](#usage)
- [Network Topology](#network-topology)
- [Protocol Details](#protocol-details)
- [Campus Authentication](#campus-authentication)
- [Contributing](#contributing)

---

## 🎯 Project Overview

This project implements a distributed messaging system that connects multiple university campuses through a central server. The system enables authenticated campus-to-campus communication, real-time status monitoring, and broadcast announcements using both TCP and UDP protocols.

### Supported Campuses

- Lahore
- Karachi
- Peshawar
- CFD (City Campus)
- Multan
- Islamabad

---

## ✨ Features

### Client Features

1. **Campus Authentication** - Secure login with campus-specific credentials
2. **Inter-Campus Messaging** - Send messages to specific departments in other campuses
3. **Real-time Message Reception** - Receive messages from other campuses instantly
4. **Heartbeat Monitoring** - Automatic status updates to the server every 10 seconds
5. **Broadcast Announcements** - Receive university-wide announcements
6. **Multi-threaded Architecture** - Concurrent handling of messaging, heartbeats, and announcements

### Server Features

1. **Multi-Client Support** - Handle multiple campus connections simultaneously
2. **Authentication System** - Verify campus credentials before allowing access
3. **Message Routing** - Forward messages between authenticated campuses
4. **Heartbeat Tracking** - Monitor campus connection status in real-time
5. **Admin Console** - Interactive console for server management
6. **Broadcast System** - Send announcements to all connected campuses
7. **Connection Logging** - Track all authentication attempts and message routing

---

## 🏗️ Architecture

The system uses a **client-server architecture** with the following components:

### Server (Central Hub)

- **TCP Server** (Port 9000): Handles client authentication and message routing
- **UDP Server** (Port 9001): Receives heartbeat packets from clients
- **Admin Console**: Allows real-time monitoring and broadcast messaging

### Client (Campus Terminal)

- **Thread 1**: TCP listener for incoming messages
- **Thread 2**: UDP heartbeat sender (10-second intervals)
- **Thread 3**: UDP announcement listener
- **Main Thread**: User interface and message sending

```
┌─────────────┐     TCP/UDP      ┌──────────────┐     TCP/UDP      ┌─────────────┐
│   Campus    │ ◄─────────────► │    Central   │ ◄─────────────► │   Campus    │
│   Client    │                  │    Server    │                  │   Client    │
│  (Lahore)   │                  │              │                  │  (Karachi)  │
└─────────────┘                  └──────────────┘                  └─────────────┘
                                        ▲
                                        │
                                 ┌──────┴───────┐
                                 │ Admin Console│
                                 └──────────────┘
```

---

## 💻 Technologies Used

- **Language**: C++11
- **Networking**: POSIX Sockets (BSD sockets)
- **Threading**: C++ Standard Library (`<thread>`)
- **Protocols**: TCP for messaging, UDP for heartbeats and broadcasts
- **Build System**: g++ compiler
- **Simulation Tool**: Cisco Packet Tracer (topology file included)

---

## 📁 Project Structure

```
CN_Lab_Project/
│
├── server.cpp                      # Central server implementation
├── client.cpp                      # Campus client implementation
├── CN Lab Project Topology.pkt     # Cisco Packet Tracer topology
├── CN Lab Project.pdf              # Project documentation
└── README.md                       # This file
```

---

## 🚀 Installation

### Prerequisites

- Linux/Unix environment (Ubuntu, Fedora, macOS, or WSL on Windows)
- g++ compiler with C++11 support
- POSIX-compliant operating system

### Compilation

1. **Clone or download the repository**

```bash
git clone <repository-url>
cd CN_Lab_Project
```

2. **Compile the server**

```bash
g++ -std=c++11 -pthread server.cpp -o server
```

3. **Compile the client**

```bash
g++ -std=c++11 -pthread client.cpp -o client
```

---

## 📖 Usage

### Running the Server

1. Start the server on the host machine:

```bash
./server
```

2. The server will display:

```
======================================
  SERVER STARTED
  TCP Port: 9000
  UDP Port: 9001
======================================

[UDP] Listening for heartbeats on port 9001
[ADMIN] Commands: status | announce <text> | exit
```

### Server Admin Commands

- `status` - View connection status of all campuses
- `announce <message>` - Send broadcast message to all connected campuses
- `exit` - Close admin console (server continues running)

### Running a Client

1. Start a client on any machine that can reach the server:

```bash
./client
```

2. Enter campus name when prompted:

```
Enter Campus Name (Lahore/Karachi/Peshawar/CFD/Multan/Islamabad): Lahore
```

3. The client will authenticate and display the menu:

```
[CONNECTED] to server at 127.0.0.1:9000
[AUTHENTICATED] Welcome Lahore!

===== Lahore Campus Menu =====
1. Send Message to Another Campus
2. Logout and Exit
Choice:
```

### Sending Messages

1. Select option `1` from the menu
2. Enter the target campus name (e.g., "Karachi")
3. Enter the target department (e.g., "CS Department")
4. Type your message
5. The message will be routed through the server to the destination

### Example Session

**Lahore Client sends to Karachi:**

```
Choice: 1
Target Campus: Karachi
Target Department: IT Department
Your Message: Server maintenance scheduled for tonight
[SENT] Message delivered to server
```

**Karachi Client receives:**

```
[NEW MESSAGE] [From Lahore - IT Department] Server maintenance scheduled for tonight
```

---

## 🌐 Network Topology

The project includes a Cisco Packet Tracer topology file (`CN Lab Project Topology.pkt`) that demonstrates:

- Campus network configuration
- Router and switch setup
- IP addressing scheme
- Server placement
- Inter-campus connectivity

Open the `.pkt` file in Cisco Packet Tracer to view and simulate the network design.

---

## 🔐 Campus Authentication

Each campus has a unique password for authentication:

| Campus     | Password      |
| ---------- | ------------- |
| Lahore     | NU-LHR-123    |
| Karachi    | NU-KHI-123    |
| Peshawar   | NU-PSH-123    |
| CFD        | NU-CFD-123    |
| Multan     | NU-MTN-123    |
| Islamabad  | NU-ISB-123    |

**Note:** Credentials are hardcoded for demonstration purposes. In production, use secure authentication mechanisms.

---

## 📡 Protocol Details

### TCP Communication (Port 9000)

- **Purpose**: Authentication, message routing, and reliable delivery
- **Message Format**: 
  - Authentication: `Campus:<name>;Pass:<password>;`
  - Messaging: `FROM:<source>;TO:<destination>;DEPT:<department>;MSG:<text>`
  - Logout: `LOGOUT;`

### UDP Communication (Port 9001)

- **Purpose**: Heartbeat monitoring and broadcast announcements
- **Heartbeat Format**: `HEART|Campus:<name>|TS:<timestamp>`
- **Broadcast Format**: `ANNOUNCEMENT:<message>`
- **Frequency**: Heartbeats sent every 10 seconds

### Threading Model

**Server Threads:**
- Main thread: Accept incoming TCP connections
- UDP thread: Listen for heartbeat packets
- Admin thread: Handle console commands
- Client threads: One per connected campus (detached)

**Client Threads:**
- Main thread: User interface and menu
- TCP listener: Receive incoming messages
- Heartbeat sender: Send status updates
- Announcement listener: Receive broadcasts

---

## 🛠️ Troubleshooting

### Common Issues

**1. "Cannot bind to port"**
- Ensure no other process is using ports 9000 or 9001
- Try running with elevated privileges if necessary

**2. "Cannot connect to server"**
- Verify server is running
- Check IP address in client.cpp (default: 127.0.0.1)
- Ensure firewall allows connections on ports 9000 and 9001

**3. Authentication fails**
- Verify campus name spelling (case-sensitive)
- Check password matches the predefined values

**4. Messages not received**
- Verify both campuses are authenticated
- Check server logs for routing information
- Ensure target campus name is spelled correctly

---

## 🔮 Future Enhancements

- Database integration for dynamic campus registration
- Encrypted communication (TLS/SSL)
- File transfer capabilities
- Web-based admin dashboard
- Message persistence and history
- Group messaging and channels
- Mobile client applications

---

## 📄 License

This project is part of a Computer Networks Lab assignment and is intended for educational purposes.

---

## 📧 Contact

For questions or collaboration:

- **Course**: Computer Networks Lab
- **Section**: BSCS 5B
- **Semester**: Fall 2023

---

## 🙏 Acknowledgments

- Course instructor for project guidance
- Team members for collaborative development
- POSIX socket programming references
- C++ threading documentation
