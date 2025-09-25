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
#include "mdns.h"

#include <queue>
#include <cstring>
#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <format>
#include <chrono>
#include "Config.h"
#include "Packet.h"

struct MdnsQuest {
    bool ongoing = false;
    bool failed = false;
    std::string status;
    std::vector<std::string> resolvedIPs;
    int socke = -1;
    uint16_t queryId = 0;
    char buffer[1024];

    void knowAbout(std::string ip)
    {
        auto findKnown = std::find(resolvedIPs.begin(), resolvedIPs.end(), ip);
        if (findKnown == resolvedIPs.end()) {
            resolvedIPs.push_back(ip);
        }
    }
};

class UdpSender
{
private:
    int socke = 0;
    sockaddr_in opfer{0};
    std::string error;
    MdnsQuest mdnsQuest{};
    std::queue<Packet> queue;
    std::chrono::time_point<std::chrono::steady_clock> lastProcessedAt;

public:
    std::string host;
    uint16_t port;

    std::chrono::milliseconds processInterval{400};

    explicit UdpSender(Config config) {
        host = config.wledHost;
        port = config.wledPort;
        open();
    }

    ~UdpSender() {
        close();
    }

    int send(Packet qmd) {
        if (isClosed()) {
            open();
        }
        if (mdnsQuest.ongoing) {
            queue.push(qmd);
            return 0;
        }
        const char* msg = reinterpret_cast<const char*>(&qmd);
        ssize_t sent = sendto(socke, msg, sizeof(qmd), 0, (sockaddr*)&opfer, sizeof(opfer));
        if (sent < 0) {
            throw std::runtime_error("send() failed! error: " + error);
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

    void process() {
        auto now = std::chrono::steady_clock::now();
        if (now - lastProcessedAt < processInterval) {
            return;
        }
        if (mdnsQuest.ongoing) {
            pollMdnsResponse();
        }
        if (!queue.empty()) {
            auto qmd = queue.front();
            queue.pop();
            send(qmd);
        }
        lastProcessedAt = now;
    }

    [[nodiscard]]
    bool isClosed() const {
        return socke == 0 || !error.empty();
    }

    [[nodiscard]]
    const char* status() const {
        return (mdnsQuest.ongoing ? mdnsQuest.status : error).c_str();
    }

    [[nodiscard]]
    size_t queueSize() const {
        return queue.size();
    }

private:

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
        lastProcessedAt = std::chrono::steady_clock::now();
        std::memset(&opfer, 0, sizeof(opfer));
        opfer.sin_family = AF_INET;
        opfer.sin_port = htons(port);
        if (host.ends_with(".local")) {
            initiateMdnsQuest();
            return;
        } else {
            // assumes IPv4 format
            inet_pton(AF_INET, host.c_str(), &opfer.sin_addr);
        }
        if (opfer.sin_addr.S_un.S_addr == 0) {
            error = "Cannot deal with host, is this a straight IP format? " + host;
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

    void initiateMdnsQuest() {
        mdnsQuest.ongoing = true;
        mdnsQuest.failed = false;
        mdnsQuest.socke = mdns_socket_open_ipv4(nullptr);
        if (mdnsQuest.socke < 0) {
            mdnsQuest.ongoing = false;
            mdnsQuest.failed = true;
            error = "Failed to open mDNS socket, try with IPv4";
            mdnsQuest.status = "Cannot even open mDNS socket";
            return;
        }
        std::cout << "MDNS SOCK? " << mdnsQuest.socke << std::endl;
        mdnsQuest.queryId = mdns_query_send(mdnsQuest.socke,
                                            MDNS_RECORDTYPE_A,
                                            host.c_str(),
                                            host.size(),
                                            mdnsQuest.buffer,
                                            sizeof(mdnsQuest.buffer),
                                            0);
        mdnsQuest.status = "mDNS quest ongoing...";
    }

    void pollMdnsResponse() {
        size_t count = mdns_query_recv(
                mdnsQuest.socke,
                mdnsQuest.buffer,
                sizeof(mdnsQuest.buffer),
                onResponse,
                &mdnsQuest,
                mdnsQuest.queryId
        );
        if (count > 0) {
            mdnsQuest.status = "Resolved " +
                               std::to_string(mdnsQuest.resolvedIPs.size())
                               + " address(es)";
            mdnsQuest.ongoing = false;
            mdns_socket_close(mdnsQuest.socke);
        }
    }

    static int onResponse(int sock, const struct sockaddr* from, size_t addrlen,
                          mdns_entry_type_t type, uint16_t query_id, uint16_t rtype,
                          uint16_t rclass, uint32_t ttl, const void* data, size_t size,
                          size_t name_offset, size_t name_length, size_t record_offset,
                          size_t record_length, void* user_data)
    {
        auto* quest = static_cast<MdnsQuest*>(user_data);
        if (rtype == MDNS_RECORDTYPE_A) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, data, ip, sizeof(ip));
            quest->knowAbout(std::string(ip));
        }
        return 0;
    }
};

#endif //DLTROPHY_QMAND_UPDSENDER_H
