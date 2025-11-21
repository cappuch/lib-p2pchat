P2P Chat/Data Library

Created by sketching the architecture on paper and then pumping into Gemini 3.0

Overview

- UDP transport per node with a simple poll loop
- Encrypted, signed packets with TTL and flooding-based forwarding
- LAN discovery beacons for zero-config peer info sharing
- Minimal public API via `p2p::Node` for messages and `p2p::FileTransfer` for files

Requirements

- CMake 3.15+
- A C++17 compiler (g++/clang++)
- libsodium

Install libsodium (Linux)

- Debian/Ubuntu: `sudo apt-get install libsodium-dev`
- Fedora: `sudo dnf install libsodium-devel`
- Arch: `sudo pacman -S libsodium`

Build

- `mkdir build && cd build`
- `cmake ..`
- `make -j`
- Run example: `./example`

Quick Start

- Create two nodes on localhost and bootstrap with each other
- Register message/file handlers
- Start nodes and send text or files

Code Snippet

- See `src/main.cpp` for a complete example. Core pieces:
- `p2p::Node A("127.0.0.1", 9001);`
- `p2p::Node B("127.0.0.1", 9002);`
- `A.onTypedMessage(...); B.onTypedMessage(...);`
- `A.start(); B.start();`
- `A.addPeer({B.identity().id, B.identity().publicKey, B.identity().signPublic, "127.0.0.1", 9002});`
- `A.sendText(B.identity().id, "hello");`
- `p2p::FileTransfer txA(A); txA.sendBuffer(B.identity().id, "greeting.txt", data, 1024);`

Public API

- Node
  - `Node(const std::string& ip = "", uint16_t port = 0)`
  - `void start()` / `void stop()`
  - `const Identity& identity() const`
  - `uint16_t port() const`
  - `void addPeer(const Peer&)`
  - `bool sendMessage(const PeerId&, const std::vector<uint8_t>&)`
  - `bool sendText(const PeerId&, const std::string&)`
  - `void onMessage(MessageHandler)`
  - `void onTypedMessage(TypedHandler)`

- FileTransfer
  - `explicit FileTransfer(Node&)`
  - `void onFile(FileHandler)`
  - `bool sendFile(const PeerId&, const std::string& path, size_t chunkSize = 1024)`
  - `bool sendBuffer(const PeerId&, const std::string& name, const std::vector<uint8_t>& data, size_t chunkSize = 1024)`

How It Works

- Identity: generates Curve25519 box keypair, Ed25519 sign keypair, and `PeerId = hash(public box key)`
- Packet: `sender|dest|ttl|signature|len|payload` where payload is `crypto_box` ciphertext
- Router: verifies signature, decrypts if for self, else decrements TTL and forwards
- Transport: UDP socket, non-blocking `select()`-based poll loop
- Discovery: periodic `DISC` beacons broadcast on the bound port carrying port + keys + id

Discovery and Bootstrap

- Nodes broadcast `DISC` every 2s on their local UDP port
- On receipt, peers update directory with address and keys
- You can also manually `addPeer` when you already have address+keys

Security Notes

- Uses libsodium for crypto_box and crypto_sign
- This is a minimal example; review and harden before production use

Directory Layout

- `include/p2p/Identity.hpp` – identity, keys, ids
- `include/p2p/Crypto.hpp` – sign/verify/encrypt/decrypt
- `include/p2p/Peer*.hpp` – peer types and directory
- `include/p2p/Packet.hpp` – packet model
- `include/p2p/Transport.hpp` – UDP I/O
- `include/p2p/Router.hpp` – routing
- `include/p2p/Node.hpp` – high-level API
- `include/p2p/FileTransfer.hpp` – file chunks API
- `src/*.cpp` – implementations
- `src/main.cpp` – runnable demo

Troubleshooting

- No messages: check firewall rules and that both peers added each other
- Different hosts: ensure broadcast domain and routes allow UDP
- Build errors: verify `libsodium` dev package is installed
