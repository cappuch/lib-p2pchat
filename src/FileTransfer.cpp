#include "p2p/FileTransfer.hpp"
#include "p2p/Identity.hpp"

#include <sodium.h>
#include <filesystem>
#include <cstring>

namespace p2p {

static void ensure_init() {
    static bool inited = false;
    if (!inited) { if (sodium_init() == -1) { std::abort(); } inited = true; }
}

FileTransfer::FileTransfer(Node& node) : node_(node) {
    ensure_init();
    node_.onTypedMessage([this](const PeerId& from, MessageType type, const std::vector<uint8_t>& body){
        if (type != MessageType::FILE_CHUNK) return;
        // rebuild with type prefix
        std::vector<uint8_t> data; data.reserve(1+body.size());
        data.push_back(static_cast<uint8_t>(MessageType::FILE_CHUNK));
        data.insert(data.end(), body.begin(), body.end());
        FileId id{}; uint32_t index=0,total=0; std::string name; std::vector<uint8_t> chunk;
        if (!parseChunk(data, id, index, total, name, chunk)) return;
        auto key = toHex16(id);
        auto it = in_.find(key);
        if (it == in_.end()) {
            Incoming inc; inc.name = name; inc.total = total; inc.received = 0; inc.chunks.resize(total);
            it = in_.emplace(key, std::move(inc)).first;
        }
        Incoming& inc = it->second;
        if (inc.total != total) inc.total = total;
        if (!inc.chunks[index].has_value()) {
            inc.chunks[index] = std::move(chunk);
            inc.received++;
        }
        if (inc.received == inc.total) {
            std::vector<uint8_t> file;
            for (auto& opt : inc.chunks) { file.insert(file.end(), opt->begin(), opt->end()); }
            if (onFile_) onFile_(from, inc.name, file);
            in_.erase(it);
        }
    });
}

FileTransfer::FileId FileTransfer::randomId() {
    FileId id{}; randombytes_buf(id.data(), id.size()); return id;
}

std::string FileTransfer::toHex16(const FileId& id) { return toHex(id.data(), id.size()); }

std::vector<uint8_t> FileTransfer::buildChunk(const FileId& id, uint32_t index, uint32_t total,
                                              const std::string& name, const std::vector<uint8_t>& data) {
    // msg: type,id,i,total,nameLen,name,dataLen,data,hash
    std::vector<uint8_t> msg;
    msg.reserve(1+16+4+4+1+name.size()+2+data.size()+32);
    msg.push_back(0xF1);
    msg.insert(msg.end(), id.begin(), id.end());
    auto put32 = [&](uint32_t v){ msg.push_back((v>>24)&0xFF); msg.push_back((v>>16)&0xFF); msg.push_back((v>>8)&0xFF); msg.push_back(v&0xFF); };
    put32(index); put32(total);
    uint8_t nl = static_cast<uint8_t>(std::min<size_t>(255, name.size()));
    msg.push_back(nl);
    msg.insert(msg.end(), name.begin(), name.begin()+nl);
    uint16_t dl = static_cast<uint16_t>(std::min<size_t>(65535, data.size()));
    msg.push_back((dl>>8)&0xFF); msg.push_back(dl&0xFF);
    msg.insert(msg.end(), data.begin(), data.begin()+dl);
    std::vector<uint8_t> h(32);
    crypto_generichash(h.data(), h.size(), data.data(), dl, nullptr, 0);
    msg.insert(msg.end(), h.begin(), h.end());
    return msg;
}

bool FileTransfer::parseChunk(const std::vector<uint8_t>& msg, FileId& id, uint32_t& index, uint32_t& total,
                              std::string& name, std::vector<uint8_t>& data) {
    if (msg.size() < 1+16+4+4+1+2+32) return false;
    size_t off = 0;
    if (msg[off++] != 0xF1) return false;
    std::memcpy(id.data(), msg.data()+off, 16); off += 16;
    auto get32 = [&](uint32_t& v){ if (off+4>msg.size()) return false; v = (msg[off]<<24)|(msg[off+1]<<16)|(msg[off+2]<<8)|msg[off+3]; off+=4; return true; };
    if (!get32(index) || !get32(total)) return false;
    if (off>=msg.size()) return false; uint8_t nl = msg[off++];
    if (off+nl+2>msg.size()) return false; name.assign((const char*)msg.data()+off, (size_t)nl); off += nl;
    uint16_t dl = (msg[off]<<8)|msg[off+1]; off+=2;
    if (off+dl+32>msg.size()) return false;
    const uint8_t* dptr = msg.data()+off; off += dl;
    const uint8_t* hptr = msg.data()+off; off += 32;
    std::vector<uint8_t> h(32);
    crypto_generichash(h.data(), h.size(), dptr, dl, nullptr, 0);
    if (std::memcmp(h.data(), hptr, 32) != 0) return false;
    data.assign(dptr, dptr+dl);
    return true;
}

bool FileTransfer::sendBuffer(const PeerId& dest, const std::string& name, const std::vector<uint8_t>& data, size_t chunkSize) {
    if (chunkSize == 0) chunkSize = 1024;
    FileId id = randomId();
    uint32_t total = static_cast<uint32_t>((data.size() + chunkSize - 1) / chunkSize);
    for (uint32_t i = 0; i < total; ++i) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, data.size());
        std::vector<uint8_t> chunk(data.begin()+start, data.begin()+end);
        auto body = buildChunk(id, i, total, name, chunk);
        // body already has type
        if (!node_.sendMessage(dest, body)) return false;
    }
    return true;
}

bool FileTransfer::sendFile(const PeerId& dest, const std::string& path, size_t chunkSize) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::string name = std::filesystem::path(path).filename().string();
    return sendBuffer(dest, name, buf, chunkSize);
}

} // namespace p2p
