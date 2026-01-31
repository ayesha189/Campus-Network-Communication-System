// client.cpp - Simplified Campus Client
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

#define SERVER_IP "127.0.0.1"
#define TCP_PORT 9000
#define UDP_PORT 9001

bool running = true;
string campusName;

// Thread 1: Listen for incoming TCP messages
void listenTCP(int sock) {
    char buffer[4096];
    
    while (running) {
        memset(buffer, 0, 4096);
        int n = recv(sock, buffer, 4095, 0);
        
        if (n <= 0) {
            cout << "\n[INFO] Connection closed by server\n";
            running = false;
            close(sock);
            exit(0);
        }
        
        string msg(buffer, n);
        cout << "\n[NEW MESSAGE] " << msg << "\n> ";
        fflush(stdout);
    }
}

// Thread 2: Send heartbeat every 10 seconds
void sendHeartbeat() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cout << "[ERROR] Heartbeat socket failed\n";
        return;
    }
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    
    while (running) {
        time_t now = time(0);
        string packet = "HEART|Campus:" + campusName + "|TS:" + to_string(now);
        
        sendto(sock, packet.c_str(), packet.length(), 0, 
               (sockaddr*)&serverAddr, sizeof(serverAddr));
        
        this_thread::sleep_for(chrono::seconds(10));
    }
    
    close(sock);
}

// Thread 3: Listen for UDP announcements
void listenAnnouncements() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cout << "[ERROR] Announcement socket failed\n";
        return;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)); 

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cout << "[WARN] Cannot bind for announcements\n";
        close(sock);
        return;
    }

    char buffer[4096];
    while (running) {
        memset(buffer, 0, 4096);
        int n = recv(sock, buffer, 4095, 0);

        if (n > 0) {
            string msg(buffer, n);
            cout << "\n[ANNOUNCEMENT] " << msg << "\n> ";
            fflush(stdout);
        }
    }

    close(sock);
}

// Get password for campus
string getPassword(string campus) {
    if (campus == "Lahore") return "NU-LHR-123";
    if (campus == "Karachi") return "NU-KHI-123";
    if (campus == "Peshawar") return "NU-PSH-123";
    if (campus == "CFD") return "NU-CFD-123";
    if (campus == "Multan") return "NU-MTN-123";
    if (campus == "Islamabad") return "NU-ISB-123";
    return "unknown";
}

int main() {
    // Step 1: Get campus name
    cout << "Enter Campus Name (Lahore/Karachi/Peshawar/CFD/Multan/Islamabad): ";
    cin >> campusName;
    cin.ignore(10000, '\n');
    
    // Step 2: Create TCP connection
    int tcpSock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSock < 0) {
        cout << "ERROR: Cannot create socket\n";
        return 1;
    }
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    
    if (connect(tcpSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cout << "ERROR: Cannot connect to server\n";
        close(tcpSock);
        return 1;
    }
    
    cout << "[CONNECTED] to server at " << SERVER_IP << ":" << TCP_PORT << "\n";
    
    // Step 3: Send authentication
    string password = getPassword(campusName);
    string auth = "Campus:" + campusName + ";Pass:" + password + ";";
    send(tcpSock, auth.c_str(), auth.length(), 0);
    
    // Step 4: Wait for response
    char response[64];
    memset(response, 0, 64);
    int r = recv(tcpSock, response, 63, 0);
    
    if (r <= 0) {
        cout << "ERROR: No response from server\n";
        close(tcpSock);
        return 1;
    }
    
    string authResp(response, r);
    if (authResp != "AUTH_OK") {
        cout << "ERROR: Authentication failed - " << authResp << "\n";
        close(tcpSock);
        return 1;
    }
    
    cout << "[AUTHENTICATED] Welcome " << campusName << "!\n\n";
    
    // Step 5: Start background threads
    thread t1(listenTCP, tcpSock);
    t1.detach();
    
    thread t2(sendHeartbeat);
    t2.detach();
    
    thread t3(listenAnnouncements);
    t3.detach();
    
    // Step 6: Main menu
    while (running) {
        cout << "\n===== " << campusName << " Campus Menu =====\n";
        cout << "1. Send Message to Another Campus\n";
        cout << "2. Logout and Exit\n";
        cout << "Choice: ";
        
        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string dummy;
            getline(cin, dummy);
            continue;
        }
        cin.ignore(10000, '\n');
        
        if (choice == 1) {
            // Send message
            string targetCampus, department, message;
            
            cout << "Target Campus: ";
            getline(cin, targetCampus);
            
            cout << "Target Department: ";
            getline(cin, department);
            
            cout << "Your Message: ";
            getline(cin, message);
            
            string msg = "FROM:" + campusName + 
                        ";TO:" + targetCampus + 
                        ";DEPT:" + department + 
                        ";MSG:" + message;
            
            send(tcpSock, msg.c_str(), msg.length(), 0);
            cout << "[SENT] Message delivered to server\n";
        }
        else if (choice == 2) {
            // Logout
            string logout = "LOGOUT;";
            send(tcpSock, logout.c_str(), logout.length(), 0);
            
            running = false;
            shutdown(tcpSock, SHUT_RDWR);
            close(tcpSock);
            
            cout << "\n[LOGOUT] Goodbye!\n";
            this_thread::sleep_for(chrono::milliseconds(200));
            break;
        }
        else {
            cout << "[ERROR] Invalid choice\n";
        }
    }
    
    return 0;
}