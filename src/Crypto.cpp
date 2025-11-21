#include "p2p/Crypto.hpp"

#include <sodium.h>

namespace p2p::crypto {

static void ensure_init() {
    static bool inited = false;
    if (!inited) { if (sodium_init() == -1) { std::abort(); } inited = true; }
}

std::vector<uint8_t> sign(const SignSecret& secretKey, const std::vector<uint8_t>& message) {
    ensure_init();
    std::vector<uint8_t> sig(crypto_sign_BYTES);
    unsigned long long siglen = 0;
    crypto_sign_detached(sig.data(), &siglen, message.data(), message.size(), secretKey.data());
    sig.resize(siglen);
    return sig;
}

bool verify(const SignPublic& publicKey, const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature) {
    ensure_init();
    if (signature.size() != crypto_sign_BYTES) return false;
    return crypto_sign_verify_detached(signature.data(), message.data(), message.size(), publicKey.data()) == 0;
}

std::vector<uint8_t> encrypt(const KeyBytes& senderPriv, const KeyBytes& recipientPub, const std::vector<uint8_t>& plaintext) {
    ensure_init();
    std::vector<uint8_t> out;
    out.resize(crypto_box_NONCEBYTES + crypto_box_MACBYTES + plaintext.size());
    uint8_t* nonce = out.data();
    randombytes_buf(nonce, crypto_box_NONCEBYTES);
    uint8_t* c = out.data() + crypto_box_NONCEBYTES;
    if (crypto_box_easy(c, plaintext.data(), plaintext.size(), nonce, recipientPub.data(), senderPriv.data()) != 0) {
        return {};
    }
    return out;
}

std::vector<uint8_t> decrypt(const KeyBytes& recipientPriv, const KeyBytes& senderPub, const std::vector<uint8_t>& ciphertext) {
    ensure_init();
    if (ciphertext.size() < crypto_box_NONCEBYTES + crypto_box_MACBYTES) return {};
    const uint8_t* nonce = ciphertext.data();
    const uint8_t* c = ciphertext.data() + crypto_box_NONCEBYTES;
    size_t clen = ciphertext.size() - crypto_box_NONCEBYTES;
    std::vector<uint8_t> out;
    out.resize(clen - crypto_box_MACBYTES);
    if (crypto_box_open_easy(out.data(), c, clen, nonce, senderPub.data(), recipientPriv.data()) != 0) {
        return {};
    }
    return out;
}

} // namespace p2p::crypto
