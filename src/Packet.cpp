#include "p2p/Packet.hpp"

#include <cstring>

namespace p2p {

static void write_u32(std::vector<uint8_t>& out, uint32_t v){
    out.push_back((v >> 24) & 0xFF);
    out.push_back((v >> 16) & 0xFF);
    out.push_back((v >> 8) & 0xFF);
    out.push_back(v & 0xFF);
}

static bool read_u32(const uint8_t* data, size_t len, size_t& off, uint32_t& v){
    if (off + 4 > len) return false;
    v = (data[off] << 24) | (data[off+1] << 16) | (data[off+2] << 8) | data[off+3];
    off += 4;
    return true;
}

std::vector<uint8_t> Packet::serialize() const {
    std::vector<uint8_t> out;
    out.reserve(32+32+1+64+4+payload.size());
    out.insert(out.end(), sender.begin(), sender.end());
    out.insert(out.end(), dest.begin(), dest.end());
    out.push_back(ttl);
    out.insert(out.end(), signature.begin(), signature.end());
    write_u32(out, static_cast<uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

bool Packet::deserialize(const uint8_t* data, size_t len, Packet& outp) {
    if (len < 32+32+1+64+4) return false;
    size_t off = 0;
    std::memcpy(outp.sender.data(), data + off, 32); off += 32;
    std::memcpy(outp.dest.data(), data + off, 32); off += 32;
    outp.ttl = data[off++];
    std::memcpy(outp.signature.data(), data + off, 64); off += 64;
    uint32_t plen = 0;
    if (!read_u32(data, len, off, plen)) return false;
    if (off + plen > len) return false;
    outp.payload.assign(data + off, data + off + plen);
    return true;
}

} // namespace p2p
