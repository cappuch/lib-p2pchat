#include "p2p/Identity.hpp"

#include <random>
#include <sodium.h>

namespace p2p {

static PeerId hash32(const KeyBytes& in) {
    PeerId out{};
    crypto_generichash(out.data(), out.size(), in.data(), in.size(), nullptr, 0);
    return out;
}

Identity Identity::generate() {
    static bool inited = false;
    if (!inited) { if (sodium_init() == -1) { std::abort(); } inited = true; }

    Identity ident;
    // gen box keypair
    crypto_box_keypair(ident.publicKey.data(), ident.privateKey.data());
    // gen sign keypair
    crypto_sign_keypair(ident.signPublic.data(), ident.signSecret.data());
    // id = hash(pub)
    ident.id = hash32(ident.publicKey);
    return ident;
}

std::string toHex(const uint8_t* data, size_t len) {
    static const char* hex = "0123456789abcdef";
    std::string s;
    s.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        s.push_back(hex[(data[i] >> 4) & 0xF]);
        s.push_back(hex[data[i] & 0xF]);
    }
    return s;
}

std::string toHex(const PeerId& id) { return toHex(id.data(), id.size()); }

} // namespace p2p
