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
#include <format>
#include "Config.h"

class UdpSender
{
private:
    int socke = 0;
    sockaddr_in opfer;
    std::string error;

    void open() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            error = "WSAStartup didn't work, too bad tho";
            return;
        }
#endif
        socke = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socke < 0) {
            error = "Cannot create socket";
            return;
        }
        std::memset(&opfer, 0, sizeof(opfer));
        opfer.sin_family = AF_INET;
        opfer.sin_port = htons(port);
        inet_pton(AF_INET, host, &opfer.sin_addr);
        if (opfer.sin_addr.S_un.S_addr == 0) {
            error = std::format(
                    "Cannot deal with IP \"{}\", is this a straight IP format?",
                    host
            );
            close();
        }
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
        socke = 0;
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
        if (isClosed()) {
            open();
        }
        ssize_t sent = sendto(socke, msg, size, 0, (sockaddr*)&opfer, sizeof(opfer));
        if (sent < 0) {
            throw std::runtime_error(std::format("send() failed! error: {}", error));
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

    [[nodiscard]]
    bool isClosed() const {
        return socke <= 0;
    }

    [[nodiscard]]
    const char* error_message() const {
        return error.c_str();
    }

};

#endif //DLTROPHY_QMAND_UPDSENDER_H
