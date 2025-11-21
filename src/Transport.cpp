#include "p2p/Transport.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace p2p {

Transport::Transport() {}

Transport::~Transport() {
    if (sock_ >= 0) close(sock_);
}

bool Transport::bind(const std::string& ip, uint16_t port) {
    sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) return false;

    int yes = 1;
    ::setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    ::setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip.empty() ? INADDR_ANY : ::inet_addr(ip.c_str());
    if (::bind(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) return false;

    // set non-blocking
    int flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

    // get bound port
    socklen_t slen = sizeof(addr);
    if (::getsockname(sock_, (sockaddr*)&addr, &slen) == 0) {
        boundPort_ = ntohs(addr.sin_port);
    }
    return true;
}

bool Transport::send(const std::string& ip, uint16_t port, const Packet& pkt) {
    if (sock_ < 0) return false;
    auto buf = pkt.serialize();
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ::inet_addr(ip.c_str());
    ssize_t n = ::sendto(sock_, buf.data(), buf.size(), 0, (sockaddr*)&addr, sizeof(addr));
    return n == (ssize_t)buf.size();
}

bool Transport::sendRaw(const std::string& ip, uint16_t port, const std::vector<uint8_t>& data) {
    if (sock_ < 0) return false;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ::inet_addr(ip.c_str());
    ssize_t n = ::sendto(sock_, data.data(), data.size(), 0, (sockaddr*)&addr, sizeof(addr));
    return n == (ssize_t)data.size();
}

bool Transport::sendBroadcast(uint16_t port, const std::vector<uint8_t>& data) {
    return sendRaw("255.255.255.255", port, data);
}

void Transport::poll(int timeoutMs, const PacketHandler& pktHandler, const RawHandler& rawHandler) {
    if (sock_ < 0) return;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock_, &rfds);
    timeval tv{ timeoutMs/1000, (timeoutMs%1000)*1000 };
    int r = ::select(sock_+1, &rfds, nullptr, nullptr, &tv);
    if (r <= 0) return;

    if (FD_ISSET(sock_, &rfds)) {
        uint8_t buf[2048];
        sockaddr_in src{}; socklen_t slen = sizeof(src);
        ssize_t n = ::recvfrom(sock_, buf, sizeof(buf), 0, (sockaddr*)&src, &slen);
        if (n > 0) {
            std::string fromIp = ::inet_ntoa(src.sin_addr);
            uint16_t fromPort = ntohs(src.sin_port);
            // Discovery beacons start with "DISC"
            if (n >= 4 && buf[0]=='D' && buf[1]=='I' && buf[2]=='S' && buf[3]=='C') {
                if (rawHandler) rawHandler(std::vector<uint8_t>(buf, buf + n), fromIp, fromPort);
            } else {
                Packet pkt{};
                if (Packet::deserialize(buf, (size_t)n, pkt)) {
                    if (pktHandler) pktHandler(pkt, fromIp, fromPort);
                }
            }
        }
    }
}

} // namespace p2p
