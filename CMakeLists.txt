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
find_package(unofficial-sodium CONFIG)
if (unofficial-sodium_FOUND)
  target_link_libraries(p2pchat PUBLIC unofficial-sodium::sodium)
else()
  find_path(SODIUM_INCLUDE_DIR sodium.h
      PATHS
      /usr/include
      /usr/local/include
      /opt/homebrew/include
      /opt/local/include
  )
  
  find_library(SODIUM_LIB sodium
      PATHS
      /usr/lib
      /usr/local/lib
      /opt/homebrew/lib
      /opt/local/lib
  )

  if (SODIUM_INCLUDE_DIR AND SODIUM_LIB)
      target_include_directories(p2pchat PUBLIC ${SODIUM_INCLUDE_DIR})
      target_link_libraries(p2pchat PUBLIC ${SODIUM_LIB})
  else()
      find_package(PkgConfig)
      if (PkgConfig_FOUND)
          pkg_check_modules(SODIUM libsodium)
          if (SODIUM_FOUND)
              target_include_directories(p2pchat PUBLIC ${SODIUM_INCLUDE_DIRS})
              target_link_libraries(p2pchat PUBLIC ${SODIUM_LIBRARIES})
          else()
              message(WARNING "libsodium not found; attempting to link by name")
              target_link_libraries(p2pchat PUBLIC sodium)
          endif()
      else()
          message(WARNING "libsodium not found; attempting to link by name")
          target_link_libraries(p2pchat PUBLIC sodium)
      endif()
  endif()
endif()

if(WIN32)
    target_link_libraries(p2pchat PUBLIC ws2_32)
endif()
