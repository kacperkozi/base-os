cmake_minimum_required(VERSION 3.16)  # Ubuntu 20.04 compatible
project(base_os_tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fixed FTXUI version for reproducible builds
set(FTXUI_VERSION "6.1.9" CACHE STRING "FTXUI version to use")

# Use FetchContent as the primary dependency management method
include(FetchContent)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG        v${FTXUI_VERSION}
  GIT_SHALLOW    TRUE
)

# Configure FTXUI options for a minimal build
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS_FUZZER OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ftxui)

# QR Code generation using Nayuki library (included in source)
# The Nayuki QR Code generator is MIT licensed and included directly
# No external dependencies required - qrcodegen.cpp is compiled with the project

# Main executable
add_executable(base_os_tui
  src/main.cpp
  src/router.cpp
  src/views.cpp
  src/navigation.cpp
  src/qr_generator.cpp
  src/qr_viewer.cpp
  src/qrcodegen.cpp
  src/wallet_detector.cpp
  src/logger.cpp
  src/state.cpp
  src/validation.cpp
  src/config.cpp
)

# Include directories
target_include_directories(base_os_tui 
  PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Link libraries
target_link_libraries(base_os_tui 
  PRIVATE 
    ftxui::component 
    ftxui::dom 
    ftxui::screen
)

# Platform-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(base_os_tui PRIVATE pthread)
  
  # USB device detection libraries
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(LIBUSB QUIET libusb-1.0)
    if(LIBUSB_FOUND)
      target_link_libraries(base_os_tui PRIVATE ${LIBUSB_LIBRARIES})
      target_include_directories(base_os_tui PRIVATE ${LIBUSB_INCLUDE_DIRS})
      target_compile_options(base_os_tui PRIVATE ${LIBUSB_CFLAGS_OTHER})
      message(STATUS "Found libusb-1.0 via pkg-config: ${LIBUSB_LIBRARIES}")
    else()
      # Fallback to find_library
      find_library(LIBUSB_LIB libusb-1.0)
      if(LIBUSB_LIB)
        target_link_libraries(base_os_tui PRIVATE ${LIBUSB_LIB})
        message(STATUS "Found libusb-1.0: ${LIBUSB_LIB}")
      else()
        message(WARNING "libusb-1.0 not found - USB device detection may not work")
      endif()
    endif()
  else()
    # Fallback to find_library if pkg-config is not available
    find_library(LIBUSB_LIB libusb-1.0)
    if(LIBUSB_LIB)
      target_link_libraries(base_os_tui PRIVATE ${LIBUSB_LIB})
      message(STATUS "Found libusb-1.0: ${LIBUSB_LIB}")
    else()
      message(WARNING "libusb-1.0 not found - USB device detection may not work")
    endif()
  endif()
  
  # Link filesystem library for C++17 compatibility on older systems
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    target_link_libraries(base_os_tui PRIVATE stdc++fs)
  endif()
  
  # For Debian packaging
  set_target_properties(base_os_tui PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    INSTALL_RPATH "$ORIGIN/../lib"
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # macOS specific libraries
  find_library(IOKIT_LIB IOKit)
  find_library(COREFOUNDATION_LIB CoreFoundation)
  
  # Look for libusb in common Homebrew locations
  find_library(LIBUSB_LIB 
    NAMES libusb-1.0
    PATHS
      /opt/homebrew/lib
      /usr/local/lib
      /opt/homebrew/Cellar/libusb/*/lib
  )
  
  if(IOKIT_LIB AND COREFOUNDATION_LIB)
    target_link_libraries(base_os_tui PRIVATE ${IOKIT_LIB} ${COREFOUNDATION_LIB})
    message(STATUS "Found macOS IOKit libraries")
  endif()
  
  if(LIBUSB_LIB)
    target_link_libraries(base_os_tui PRIVATE ${LIBUSB_LIB})
    # Add include directories for libusb
    target_include_directories(base_os_tui PRIVATE 
      /opt/homebrew/include
      /usr/local/include
      /opt/homebrew/Cellar/libusb/*/include
    )
    message(STATUS "Found macOS libusb: ${LIBUSB_LIB}")
  else()
    message(WARNING "libusb-1.0 not found on macOS - USB device detection may not work")
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Windows specific libraries
  target_link_libraries(base_os_tui PRIVATE setupapi)
  message(STATUS "Linked Windows setupapi for USB detection")
endif()

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(base_os_tui PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
  target_compile_options(base_os_tui PRIVATE /W4)
endif()

# Export compile commands for development tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Installation rules for Debian packaging
include(GNUInstallDirs)

install(TARGETS base_os_tui
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install configuration file template
install(FILES "${CMAKE_CURRENT_LIST_DIR}/config/default_config.json"
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/base-os-tui
  RENAME config.json.example
  OPTIONAL
)

# Create directories for runtime files
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_LOCALSTATEDIR}/log/base-os-tui)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_LOCALSTATEDIR}/lib/base-os-tui)

# Summary
message(STATUS "")
message(STATUS "=== Base OS TUI Configuration ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "FTXUI Version: ${FTXUI_VERSION}")
message(STATUS "FTXUI Source: FetchContent (GitHub)")
message(STATUS "QR Code Library: Nayuki (MIT licensed, included)")
message(STATUS "==================================")
message(STATUS "")