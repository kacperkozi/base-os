cmake_minimum_required(VERSION 3.20)
project(base_os_tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FTXUI_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party/FTXUI")
if(EXISTS "${FTXUI_DIR}/CMakeLists.txt")
  add_subdirectory("${FTXUI_DIR}" ftxui)
else()
  message(FATAL_ERROR "FTXUI not found at ${FTXUI_DIR}. Clone it to ui/third_party/FTXUI.")
endif()

add_executable(base_os_tui
  src/main.cpp
  src/views.cpp
  src/state.cpp
  src/validation.cpp
)
target_include_directories(base_os_tui PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(base_os_tui PRIVATE ftxui::component ftxui::dom ftxui::screen)

if (CMAKE_EXPORT_COMPILE_COMMANDS)
  message(STATUS "Compile commands enabled")
endif()

