#include "p2p/Node.hpp"
#include "p2p/Identity.hpp"
#include "p2p/Peer.hpp"
#include "p2p/FileTransfer.hpp"

#include <iostream>
#include <thread>

using namespace p2p;

int main() {
    // create two nodes
    Node A("127.0.0.1", 9001);
    Node B("127.0.0.1", 9002);

    A.onTypedMessage([](const PeerId& from, MessageType type, const std::vector<uint8_t>& payload){
        if (type != MessageType::TEXT) return;
        std::cout << "A received from " << toHex(from) << ": " << std::string(payload.begin(), payload.end()) << "\n";
    });
    B.onTypedMessage([](const PeerId& from, MessageType type, const std::vector<uint8_t>& payload){
        if (type != MessageType::TEXT) return;
        std::cout << "B received from " << toHex(from) << ": " << std::string(payload.begin(), payload.end()) << "\n";
    });

    A.start();
    B.start();

    // manual bootstrap
    Peer pA{A.identity().id, A.identity().publicKey, A.identity().signPublic, "127.0.0.1", 9001};
    Peer pB{B.identity().id, B.identity().publicKey, B.identity().signPublic, "127.0.0.1", 9002};
    A.addPeer(pB);
    B.addPeer(pA);

    std::cout << "Node A id=" << toHex(A.identity().id) << " port=" << A.port() << "\n";
    std::cout << "Node B id=" << toHex(B.identity().id) << " port=" << B.port() << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    A.sendText(B.identity().id, "hello from A");

    // file transfer demo
    p2p::FileTransfer txA(A);
    p2p::FileTransfer txB(B);
    txB.onFile([](const PeerId& from, const std::string& name, const std::vector<uint8_t>& data){
        std::cout << "B got file from " << toHex(from) << ": " << name << ", size=" << data.size() << "\n";
        std::cout << "  preview: " << std::string(data.begin(), data.begin()+std::min<size_t>(data.size(), 40)) << "\n";
    });

    std::vector<uint8_t> fileData{'H','e','l','l','o',' ','F','i','l','e',' ','T','r','a','n','s','f','e','r'};
    txA.sendBuffer(B.identity().id, "greeting.txt", fileData, 8);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    A.stop();
    B.stop();
    return 0;
}
