cmake_minimum_required(VERSION 3.12)
project(p2pchat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(p2pchat
    src/Identity.cpp
    src/Crypto.cpp
    src/PeerDirectory.cpp
    src/Packet.cpp
    src/Transport.cpp
    src/Router.cpp
    src/Node.cpp
    src/FileTransfer.cpp
)

target_include_directories(p2pchat PUBLIC include)

add_executable(example src/main.cpp)
target_link_libraries(example PRIVATE p2pchat)

# Link libsodium
find_library(SODIUM_LIB sodium)
if (SODIUM_LIB)
  target_link_libraries(p2pchat PUBLIC ${SODIUM_LIB})
else()
  message(WARNING "libsodium not found via find_library; attempting to link by name")
  target_link_libraries(p2pchat PUBLIC sodium)
endif()
