// server.cpp - Simplified Campus Server
#include <iostream>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

#define TCP_PORT 9000
#define UDP_PORT 9001

// Global data
map<string, string> passwords = {
    {"Lahore", "NU-LHR-123"}, {"Karachi", "NU-KHI-123"},
    {"Peshawar", "NU-PSH-123"}, {"CFD", "NU-CFD-123"},
    {"Multan", "NU-MTN-123"}, {"Islamabad", "NU-ISB-123"}
};

map<string, int> campusSockets;      // campus -> socket
map<string, time_t> lastSeen;        // campus -> last heartbeat time
mutex mtx;

// Get current time as string
string getTime() {
    time_t now = time(0);
    string t = ctime(&now);
    if (t.back() == '\n') t.pop_back();
    return t;
}

// Extract value after a key (e.g., "FROM:Lahore;" -> "Lahore")
string getValue(string text, string key) {
    size_t pos = text.find(key);
    if (pos == string::npos) return "";
    
    pos += key.length();
    size_t end = text.find(";", pos);
    
    if (end == string::npos)
        return text.substr(pos);
    return text.substr(pos, end - pos);
}

// Send UDP broadcast to all clients
void sendBroadcast(string message) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cout << "[ERROR] Cannot create UDP socket\n";
        return;
    }

    // Enable broadcast
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);

    // Broadcast address — important fix
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    string packet = "ANNOUNCEMENT:" + message;
    sendto(sock, packet.c_str(), packet.length(), 0,
           (sockaddr*)&addr, sizeof(addr));

    close(sock);
}


// Admin console thread
void adminThread() {
    cout << "[ADMIN] Commands: status | announce <text> | exit\n";
    
    while (true) {
        cout << "\n[ADMIN] > ";
        string line;
        getline(cin, line);
        
        if (line.empty()) continue;
        
        if (line == "exit") {
            cout << "[ADMIN] Console closed. Server still running.\n";
            break;
        }
        else if (line == "status") {
            mtx.lock();
            cout << "\n===== Campus Status =====\n";
            for (auto &p : passwords) {
                string campus = p.first;
                cout << campus << ": ";
                
                if (campusSockets.count(campus)) {
                    cout << "CONNECTED";
                    if (lastSeen.count(campus)) {
                        time_t t = lastSeen[campus];
                        cout << " (Heartbeat: " << ctime(&t) << ")";
                    }
                } else {
                    cout << "OFFLINE";
                }
                cout << "\n";
            }
            cout << "========================\n";
            mtx.unlock();
        }
        else if (line.find("announce ") == 0) {
            string msg = line.substr(9);
            sendBroadcast(msg);
            cout << "[ADMIN] Broadcast sent!\n";
        }
        else {
            cout << "[ADMIN] Unknown command\n";
        }
    }
}

// Handle one campus client
void handleClient(int sock, string ip) {
    char buffer[4096];
    
    // Step 1: Receive authentication
    memset(buffer, 0, 4096);
    int n = recv(sock, buffer, 4095, 0);
    if (n <= 0) {
        close(sock);
        return;
    }
    
    string authMsg(buffer, n);
    string campus = getValue(authMsg, "Campus:");
    string pass = getValue(authMsg, "Pass:");
    
    cout << "[AUTH] " << campus << " trying to connect from " << ip << "\n";
    
    // Step 2: Validate credentials
    if (passwords.count(campus) == 0 || passwords[campus] != pass) {
        string fail = "AUTH_FAIL";
        send(sock, fail.c_str(), fail.length(), 0);
        cout << "[AUTH] REJECTED: " << campus << "\n";
        close(sock);
        return;
    }
    
    // Step 3: Send success
    string ok = "AUTH_OK";
    send(sock, ok.c_str(), ok.length(), 0);
    
    mtx.lock();
    campusSockets[campus] = sock;
    lastSeen[campus] = time(0);
    mtx.unlock();
    
    cout << "[AUTH] SUCCESS: " << campus << " connected\n";
    
    // Step 4: Receive messages loop
    while (true) {
        memset(buffer, 0, 4096);
        n = recv(sock, buffer, 4095, 0);
        
        if (n <= 0) {
            cout << "[DISCONNECT] " << campus << "\n";
            mtx.lock();
            campusSockets.erase(campus);
            mtx.unlock();
            close(sock);
            break;
        }
        
        string msg(buffer, n);
        
        // Check for logout
        if (msg.find("LOGOUT") == 0) {
            cout << "[LOGOUT] " << campus << "\n";
            mtx.lock();
            campusSockets.erase(campus);
            mtx.unlock();
            close(sock);
            break;
        }
        
        // Parse message
        string from = getValue(msg, "FROM:");
        string to = getValue(msg, "TO:");
        string dept = getValue(msg, "DEPT:");
        string text = getValue(msg, "MSG:");
        
        if (to.empty()) {
            cout << "[MESSAGE] " << campus << ": " << msg << "\n";
            continue;
        }
        
        cout << "[ROUTE] " << from << " -> " << to << " (Dept: " << dept << ")\n";
        
        // Forward to target campus
        mtx.lock();
        if (campusSockets.count(to)) {
            int destSock = campusSockets[to];
            string forward = "[From " + from + " - " + dept + "] " + text;
            send(destSock, forward.c_str(), forward.length(), 0);
            cout << "[DELIVERED] to " << to << "\n";
        } else {
            cout << "[FAILED] " << to << " not connected\n";
        }
        mtx.unlock();
    }
}

// UDP heartbeat listener
void udpThread() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cout << "[ERROR] UDP socket failed\n";
        return;
    }
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cout << "[ERROR] UDP bind failed on port " << UDP_PORT << "\n";
        close(sock);
        return;
    }
    
    cout << "[UDP] Listening for heartbeats on port " << UDP_PORT << "\n";
    
    char buffer[1024];
    sockaddr_in sender;
    socklen_t len = sizeof(sender);
    
    while (true) {
        memset(buffer, 0, 1024);
        int n = recvfrom(sock, buffer, 1023, 0, (sockaddr*)&sender, &len);
        
        if (n > 0) {
            string hb(buffer, n);
            
            // Extract campus name
            string campus = getValue(hb, "Campus:");
            
            if (!campus.empty()) {
                mtx.lock();
                lastSeen[campus] = time(0);
                mtx.unlock();
                
                cout << "[HEARTBEAT] " << campus << " at " << getTime() << "\n";
            }
        }
    }
    
    close(sock);
}

// Main function
int main() {
    // Create TCP socket
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        cout << "ERROR: Cannot create socket\n";
        return 1;
    }
    
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cout << "ERROR: Cannot bind to port " << TCP_PORT << "\n";
        return 1;
    }
    
    if (listen(serverSock, 10) < 0) {
        cout << "ERROR: Cannot listen\n";
        return 1;
    }
    
    cout << "======================================\n";
    cout << "  SERVER STARTED\n";
    cout << "  TCP Port: " << TCP_PORT << "\n";
    cout << "  UDP Port: " << UDP_PORT << "\n";
    cout << "======================================\n\n";
    
    // Start UDP thread
    thread t1(udpThread);
    t1.detach();
    
    // Start admin thread
    thread t2(adminThread);
    t2.detach();
    
    // Accept clients
    while (true) {
        sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);
        int clientSock = accept(serverSock, (sockaddr*)&clientAddr, &len);
        
        if (clientSock < 0) continue;
        
        string ip = inet_ntoa(clientAddr.sin_addr);
        cout << "[NEW CONNECTION] from " << ip << "\n";
        
        thread t3(handleClient, clientSock, ip);
        t3.detach();
    }
    
    close(serverSock);
    return 0;
}