#pragma once

#include "p2p/Identity.hpp"

#include <vector>
#include <string>
#include <cstdint>

namespace p2p::crypto {

// sign message
std::vector<uint8_t> sign(const SignSecret& secretKey, const std::vector<uint8_t>& message);

// verify signature
bool verify(const SignPublic& publicKey, const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature);

// encrypt with crypto_box
std::vector<uint8_t> encrypt(const KeyBytes& senderPriv, const KeyBytes& recipientPub, const std::vector<uint8_t>& plaintext);

// decrypt with crypto_box
std::vector<uint8_t> decrypt(const KeyBytes& recipientPriv, const KeyBytes& senderPub, const std::vector<uint8_t>& ciphertext);

} // namespace p2p::crypto
