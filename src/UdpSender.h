//
// Created by qm210 on 19.09.2025.
//

#ifndef DLTROPHY_QMAND_UPDSENDER_H
#define DLTROPHY_QMAND_UPDSENDER_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
  #include <arpa/inet.h>
  #include <unistd.h>
#endif

#include <cstring>
#include <iostream>
#include <cstdint>
#include "Config.h"

class UdpSender
{
private:
    int socke;
    sockaddr_in opfer;

    void open() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup didn't work, too bad tho");
        }
#endif
        socke = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socke < 0) {
            throw std::runtime_error("Cannot create socket");
        }
        std::memset(&opfer, 0, sizeof(opfer));
        opfer.sin_family = AF_INET;
        opfer.sin_port = htons(port);
        inet_pton(AF_INET, host, &opfer.sin_addr);
    }

    void close() {
#ifdef _WIN32
        if (socke > 0) {
            closesocket(socke);
        }
        WSACleanup();
#else
        close(socke);
#endif
    }

public:
    const char* host;
    uint16_t port;

    explicit UdpSender(Config config) {
        host = config.wledHost.c_str();
        port = config.wledPort;
        open();
    }

    ~UdpSender() {
        std::cout << "[DEBUG] ~UdpSender() called, closing " << socke << std::endl;
        close();
    }

    int send(const char* msg, size_t size) {
        ssize_t sent = sendto(socke, msg, size, 0, (sockaddr*)&opfer, sizeof(opfer));
        if (sent < 0) {
            throw std::runtime_error("send() failed!");
        }
        return sent;
    }

    void update(Config config) {
        if (host == config.wledHost && port == config.wledPort) {
            return;
        }
        host = config.wledHost.c_str();
        port = config.wledPort;
        close();
        open();
    }
};

#endif //DLTROPHY_QMAND_UPDSENDER_H
