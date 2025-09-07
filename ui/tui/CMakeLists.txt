cmake_minimum_required(VERSION 3.16)  # Ubuntu 20.04 compatible
project(base_os_tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options for different deployment scenarios (prioritize .deb package)
option(USE_DEB_FTXUI "Use FTXUI from .deb package in third-party/" ON)
option(USE_SYSTEM_FTXUI "Use system-installed FTXUI" OFF)
option(USE_VENDORED_FTXUI "Use vendored FTXUI tarball" OFF)
option(USE_FETCHCONTENT_FTXUI "Fetch FTXUI from GitHub (not for production)" OFF)

# Fixed FTXUI version for reproducible builds
set(FTXUI_VERSION "6.1.9" CACHE STRING "FTXUI version to use")

# First, try to use FTXUI from .deb package in third-party/
if(USE_DEB_FTXUI AND NOT FTXUI_FOUND)
  message(STATUS "Looking for FTXUI .deb package...")
  
  # Check architecture compatibility
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    message(WARNING "ARM64 architecture detected - .deb package is x86_64 only")
    message(STATUS "Skipping .deb package, using FetchContent for ARM64 compatibility")
    set(USE_DEB_FTXUI OFF)
    set(USE_VENDORED_FTXUI OFF)
    set(USE_FETCHCONTENT_FTXUI ON)
  else()
    # Look for .deb file in third-party directory
  set(DEB_PATHS
    "${CMAKE_CURRENT_LIST_DIR}/../third-party/ftxui-${FTXUI_VERSION}-Linux.deb"
    "${CMAKE_CURRENT_LIST_DIR}/../third_party/ftxui-${FTXUI_VERSION}-Linux.deb"
    "${CMAKE_CURRENT_LIST_DIR}/third-party/ftxui-${FTXUI_VERSION}-Linux.deb"
  )
  
  foreach(deb_path ${DEB_PATHS})
    if(EXISTS "${deb_path}")
      message(STATUS "Found FTXUI .deb package: ${deb_path}")
      
      # Extract .deb package to build directory
      set(DEB_EXTRACT_DIR "${CMAKE_CURRENT_BINARY_DIR}/ftxui-deb-extracted")
      
      if(NOT EXISTS "${DEB_EXTRACT_DIR}")
        message(STATUS "Extracting FTXUI .deb package...")
        file(MAKE_DIRECTORY "${DEB_EXTRACT_DIR}")
        
        # Extract .deb package (ar archive containing data.tar.*)
        execute_process(
          COMMAND ar x "${deb_path}"
          WORKING_DIRECTORY "${DEB_EXTRACT_DIR}"
          RESULT_VARIABLE ar_result
          OUTPUT_QUIET ERROR_QUIET
        )
        
        if(ar_result EQUAL 0)
          # Find and extract data archive
          file(GLOB data_archives "${DEB_EXTRACT_DIR}/data.tar.*")
          if(data_archives)
            list(GET data_archives 0 data_archive)
            execute_process(
              COMMAND ${CMAKE_COMMAND} -E tar xf "${data_archive}"
              WORKING_DIRECTORY "${DEB_EXTRACT_DIR}"
              RESULT_VARIABLE tar_result
              OUTPUT_QUIET ERROR_QUIET
            )
            
            if(tar_result EQUAL 0)
              message(STATUS "Successfully extracted .deb package")
              
              # Look for FTXUI files in extracted package
              find_path(FTXUI_INCLUDE_DIR
                NAMES ftxui/component/component.hpp
                PATHS "${DEB_EXTRACT_DIR}/usr/include"
                NO_DEFAULT_PATH
              )
              
              find_library(FTXUI_COMPONENT_LIB
                NAMES ftxui-component
                PATHS "${DEB_EXTRACT_DIR}/usr/lib" "${DEB_EXTRACT_DIR}/usr/lib/x86_64-linux-gnu"
                NO_DEFAULT_PATH
              )
              
              find_library(FTXUI_DOM_LIB
                NAMES ftxui-dom
                PATHS "${DEB_EXTRACT_DIR}/usr/lib" "${DEB_EXTRACT_DIR}/usr/lib/x86_64-linux-gnu"
                NO_DEFAULT_PATH
              )
              
              find_library(FTXUI_SCREEN_LIB
                NAMES ftxui-screen
                PATHS "${DEB_EXTRACT_DIR}/usr/lib" "${DEB_EXTRACT_DIR}/usr/lib/x86_64-linux-gnu"
                NO_DEFAULT_PATH
              )
              
              if(FTXUI_INCLUDE_DIR AND FTXUI_COMPONENT_LIB AND FTXUI_DOM_LIB AND FTXUI_SCREEN_LIB)
                message(STATUS "Found FTXUI in .deb package:")
                message(STATUS "  Include: ${FTXUI_INCLUDE_DIR}")
                message(STATUS "  Component: ${FTXUI_COMPONENT_LIB}")
                message(STATUS "  DOM: ${FTXUI_DOM_LIB}")
                message(STATUS "  Screen: ${FTXUI_SCREEN_LIB}")
                
                # Create imported targets for .deb FTXUI
                add_library(ftxui::screen STATIC IMPORTED)
                set_target_properties(ftxui::screen PROPERTIES
                  IMPORTED_LOCATION "${FTXUI_SCREEN_LIB}"
                  INTERFACE_INCLUDE_DIRECTORIES "${FTXUI_INCLUDE_DIR}"
                )
                
                add_library(ftxui::dom STATIC IMPORTED)
                set_target_properties(ftxui::dom PROPERTIES
                  IMPORTED_LOCATION "${FTXUI_DOM_LIB}"
                  INTERFACE_INCLUDE_DIRECTORIES "${FTXUI_INCLUDE_DIR}"
                  INTERFACE_LINK_LIBRARIES "ftxui::screen"
                )
                
                add_library(ftxui::component STATIC IMPORTED)
                set_target_properties(ftxui::component PROPERTIES
                  IMPORTED_LOCATION "${FTXUI_COMPONENT_LIB}"
                  INTERFACE_INCLUDE_DIRECTORIES "${FTXUI_INCLUDE_DIR}"
                  INTERFACE_LINK_LIBRARIES "ftxui::dom"
                )
                
                set(FTXUI_FOUND TRUE)
                break()
              endif()
            endif()
          endif()
        endif()
      else()
        message(STATUS "Using previously extracted .deb package")
        # Try to find in already extracted directory
        find_path(FTXUI_INCLUDE_DIR
          NAMES ftxui/component/component.hpp
          PATHS "${DEB_EXTRACT_DIR}/usr/include"
          NO_DEFAULT_PATH
        )
        
        if(FTXUI_INCLUDE_DIR)
          # Re-setup the targets if already extracted
          find_library(FTXUI_COMPONENT_LIB NAMES ftxui-component PATHS "${DEB_EXTRACT_DIR}/usr/lib" "${DEB_EXTRACT_DIR}/usr/lib/x86_64-linux-gnu" NO_DEFAULT_PATH)
          find_library(FTXUI_DOM_LIB NAMES ftxui-dom PATHS "${DEB_EXTRACT_DIR}/usr/lib" "${DEB_EXTRACT_DIR}/usr/lib/x86_64-linux-gnu" NO_DEFAULT_PATH)
          find_library(FTXUI_SCREEN_LIB NAMES ftxui-screen PATHS "${DEB_EXTRACT_DIR}/usr/lib" "${DEB_EXTRACT_DIR}/usr/lib/x86_64-linux-gnu" NO_DEFAULT_PATH)
          
          if(FTXUI_COMPONENT_LIB AND FTXUI_DOM_LIB AND FTXUI_SCREEN_LIB)
            add_library(ftxui::screen STATIC IMPORTED)
            set_target_properties(ftxui::screen PROPERTIES IMPORTED_LOCATION "${FTXUI_SCREEN_LIB}" INTERFACE_INCLUDE_DIRECTORIES "${FTXUI_INCLUDE_DIR}")
            add_library(ftxui::dom STATIC IMPORTED)
            set_target_properties(ftxui::dom PROPERTIES IMPORTED_LOCATION "${FTXUI_DOM_LIB}" INTERFACE_INCLUDE_DIRECTORIES "${FTXUI_INCLUDE_DIR}" INTERFACE_LINK_LIBRARIES "ftxui::screen")
            add_library(ftxui::component STATIC IMPORTED)
            set_target_properties(ftxui::component PROPERTIES IMPORTED_LOCATION "${FTXUI_COMPONENT_LIB}" INTERFACE_INCLUDE_DIRECTORIES "${FTXUI_INCLUDE_DIR}" INTERFACE_LINK_LIBRARIES "ftxui::dom")
            set(FTXUI_FOUND TRUE)
            break()
          endif()
        endif()
      endif()
    endif()
  endforeach()
  
  if(NOT FTXUI_FOUND)
    message(WARNING ".deb package not found, extraction failed, or architecture mismatch")
    message(STATUS "Falling back to vendored FTXUI")
    set(USE_DEB_FTXUI OFF)
    set(USE_VENDORED_FTXUI ON)
  endif()
  endif()  # Close the else() block for architecture check
endif()

# Second, try to use system-installed FTXUI
if(USE_SYSTEM_FTXUI)
  message(STATUS "Looking for system-installed FTXUI...")
  
  # Check standard Debian library paths
  find_path(FTXUI_INCLUDE_DIR 
    NAMES ftxui/component/component.hpp
    PATHS 
      /usr/include
      /usr/local/include
      /opt/ftxui/include
  )
  
  find_library(FTXUI_COMPONENT_LIB
    NAMES ftxui-component
    PATHS
      /usr/lib
      /usr/local/lib
      /usr/lib/x86_64-linux-gnu
      /usr/lib/aarch64-linux-gnu
      /opt/ftxui/lib
  )
  
  find_library(FTXUI_DOM_LIB
    NAMES ftxui-dom
    PATHS
      /usr/lib
      /usr/local/lib
      /usr/lib/x86_64-linux-gnu
      /usr/lib/aarch64-linux-gnu
      /opt/ftxui/lib
  )
  
  find_library(FTXUI_SCREEN_LIB
    NAMES ftxui-screen
    PATHS
      /usr/lib
      /usr/local/lib
      /usr/lib/x86_64-linux-gnu
      /usr/lib/aarch64-linux-gnu
      /opt/ftxui/lib
  )
  
  if(FTXUI_INCLUDE_DIR AND FTXUI_COMPONENT_LIB AND FTXUI_DOM_LIB AND FTXUI_SCREEN_LIB)
    message(STATUS "Found system FTXUI:")
    message(STATUS "  Include: ${FTXUI_INCLUDE_DIR}")
    message(STATUS "  Component: ${FTXUI_COMPONENT_LIB}")
    message(STATUS "  DOM: ${FTXUI_DOM_LIB}")
    message(STATUS "  Screen: ${FTXUI_SCREEN_LIB}")
    
    # Create imported targets for system FTXUI
    add_library(ftxui::screen STATIC IMPORTED)
    set_target_properties(ftxui::screen PROPERTIES
      IMPORTED_LOCATION "${FTXUI_SCREEN_LIB}"
      INTERFACE_INCLUDE_DIRECTORIES "${FTXUI_INCLUDE_DIR}"
    )
    
    add_library(ftxui::dom STATIC IMPORTED)
    set_target_properties(ftxui::dom PROPERTIES
      IMPORTED_LOCATION "${FTXUI_DOM_LIB}"
      INTERFACE_INCLUDE_DIRECTORIES "${FTXUI_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES "ftxui::screen"
    )
    
    add_library(ftxui::component STATIC IMPORTED)
    set_target_properties(ftxui::component PROPERTIES
      IMPORTED_LOCATION "${FTXUI_COMPONENT_LIB}"
      INTERFACE_INCLUDE_DIRECTORIES "${FTXUI_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES "ftxui::dom"
    )
    
    set(FTXUI_FOUND TRUE)
  else()
    message(WARNING "System FTXUI not found, falling back to vendored version")
    set(USE_SYSTEM_FTXUI OFF)
    set(USE_VENDORED_FTXUI ON)
  endif()
endif()

# Use vendored FTXUI (from tarball or local directory)
if(USE_VENDORED_FTXUI AND NOT FTXUI_FOUND)
  message(STATUS "Using vendored FTXUI...")
  
  # Look for vendored FTXUI in these locations (prioritize third-party)
  set(FTXUI_VENDOR_PATHS
    "${CMAKE_CURRENT_LIST_DIR}/../third-party/FTXUI"
    "${CMAKE_CURRENT_LIST_DIR}/../third_party/FTXUI"
    "${CMAKE_CURRENT_LIST_DIR}/third-party/ftxui"
    "${CMAKE_CURRENT_LIST_DIR}/vendor/ftxui-${FTXUI_VERSION}"
    "${CMAKE_CURRENT_LIST_DIR}/vendor/FTXUI"
  )
  
  foreach(vendor_path ${FTXUI_VENDOR_PATHS})
    if(EXISTS "${vendor_path}/CMakeLists.txt")
      message(STATUS "Found vendored FTXUI at: ${vendor_path}")
      
      # Configure FTXUI build options for minimal build
      set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
      set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
      set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
      set(FTXUI_BUILD_TESTS_FUZZER OFF CACHE BOOL "" FORCE)
      set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
      
      add_subdirectory("${vendor_path}" ftxui)
      set(FTXUI_FOUND TRUE)
      break()
    endif()
  endforeach()
  
  if(NOT FTXUI_FOUND)
    # Try to extract from vendored tarball
    set(FTXUI_TARBALL "${CMAKE_CURRENT_LIST_DIR}/vendor/ftxui-${FTXUI_VERSION}.tar.gz")
    if(EXISTS "${FTXUI_TARBALL}")
      message(STATUS "Extracting FTXUI from tarball: ${FTXUI_TARBALL}")
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf "${FTXUI_TARBALL}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/vendor"
        RESULT_VARIABLE extract_result
      )
      if(extract_result EQUAL 0)
        set(vendor_path "${CMAKE_CURRENT_LIST_DIR}/vendor/ftxui-${FTXUI_VERSION}")
        if(EXISTS "${vendor_path}/CMakeLists.txt")
          add_subdirectory("${vendor_path}" ftxui)
          set(FTXUI_FOUND TRUE)
        endif()
      endif()
    endif()
  endif()
endif()

# Last resort: Use FetchContent (not recommended for production)
if(USE_FETCHCONTENT_FTXUI AND NOT FTXUI_FOUND)
  message(WARNING "Using FetchContent for FTXUI - not recommended for production!")
  message(STATUS "Fetching FTXUI v${FTXUI_VERSION} from GitHub...")
  
  include(FetchContent)
  FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v${FTXUI_VERSION}
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
  )
  
  # Configure FTXUI options
  set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(FTXUI_BUILD_TESTS_FUZZER OFF CACHE BOOL "" FORCE)
  set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  
  FetchContent_MakeAvailable(ftxui)
  set(FTXUI_FOUND TRUE)
endif()

# Error if FTXUI not found
if(NOT FTXUI_FOUND)
  message(FATAL_ERROR "FTXUI not found! Please either:\n"
    "  1. Install FTXUI system-wide (apt install libftxui-dev)\n"
    "  2. Place FTXUI source in vendor/ftxui-${FTXUI_VERSION}/\n"
    "  3. Download ftxui-${FTXUI_VERSION}.tar.gz to vendor/\n"
    "  4. Enable USE_FETCHCONTENT_FTXUI (not for production)")
endif()

# ZXing-CPP support (optional)
option(USE_SYSTEM_ZXING "Use system ZXing-CPP if available" OFF)
if(USE_SYSTEM_ZXING)
  find_package(ZXing QUIET)
  if(ZXing_FOUND)
    message(STATUS "Using system ZXing-CPP")
    set(ZXING_LIBS ZXing::ZXing)
  else()
    message(STATUS "ZXing-CPP not found, using fallback QR implementation")
    add_compile_definitions(NO_ZXING)
  endif()
else()
  add_compile_definitions(NO_ZXING)
endif()

# Main executable
add_executable(base_os_tui
  src/main.cpp
  src/hello_world.cpp
  src/simple_transaction.cpp
  # Comment out problematic files for debugging
  # src/views_minimal.cpp
  # src/views_new.cpp
  # src/views_thread_safe.cpp
  # src/state.cpp
  # src/validation.cpp
  # src/qr_generator.cpp
  # src/config.cpp
  # src/logger.cpp
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
    ${ZXING_LIBS}
)

# Platform-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(base_os_tui PRIVATE pthread)
  
  # Link filesystem library for C++17 compatibility on older systems
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    target_link_libraries(base_os_tui PRIVATE stdc++fs)
  endif()
  
  # For Debian packaging
  set_target_properties(base_os_tui PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    INSTALL_RPATH "$ORIGIN/../lib"
  )
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
message(STATUS "FTXUI Source:")
if(USE_DEB_FTXUI AND FTXUI_FOUND)
  message(STATUS "  .deb package (third-party/)")
elseif(USE_SYSTEM_FTXUI AND FTXUI_FOUND)
  message(STATUS "  System package")
elseif(USE_VENDORED_FTXUI)
  message(STATUS "  Vendored (tarball/directory)")
elseif(USE_FETCHCONTENT_FTXUI)
  message(STATUS "  FetchContent (GitHub)")
endif()
message(STATUS "System ZXing: ${USE_SYSTEM_ZXING}")
message(STATUS "Ubuntu Compatible: YES")
message(STATUS "==================================")
message(STATUS "")