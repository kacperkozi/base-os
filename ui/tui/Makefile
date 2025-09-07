# Base OS TUI Makefile
# Simplifies building and running the application

# Default build type
BUILD_TYPE ?= Release

# Build directory
BUILD_DIR ?= build

# Installation prefix
PREFIX ?= /usr/local

# Number of parallel jobs
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# CMake options
CMAKE_OPTIONS = \
	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
BLUE := \033[0;34m
NC := \033[0m # No Color

.PHONY: all build clean run debug release install uninstall test help setup deps fetch-deps

# Default target
all: build

# Help target
help:
	@echo "$(BLUE)Base OS TUI - Build System$(NC)"
	@echo ""
	@echo "$(GREEN)Available targets:$(NC)"
	@echo "  $(YELLOW)build$(NC)       - Build the application (default: Release)"
	@echo "  $(YELLOW)debug$(NC)       - Build with debug symbols"
	@echo "  $(YELLOW)release$(NC)     - Build optimized release version"
	@echo "  $(YELLOW)clean$(NC)       - Remove build directory"
	@echo "  $(YELLOW)run$(NC)         - Build and run the application"
	@echo "  $(YELLOW)install$(NC)     - Install the application to $(PREFIX)/bin"
	@echo "  $(YELLOW)uninstall$(NC)   - Remove installed application"
	@echo "  $(YELLOW)setup$(NC)       - Initial setup (fetch dependencies)"
	@echo "  $(YELLOW)fetch-deps$(NC)  - Download FTXUI dependency"
	@echo "  $(YELLOW)test$(NC)        - Run tests (if available)"
	@echo ""
	@echo "$(GREEN)Build options:$(NC)"
	@echo "  BUILD_TYPE=$(BUILD_TYPE) (Debug|Release|RelWithDebInfo|MinSizeRel)"
	@echo "  BUILD_DIR=$(BUILD_DIR)"
	@echo "  PREFIX=$(PREFIX)"
	@echo "  JOBS=$(JOBS)"
	@echo ""
	@echo "$(GREEN)Examples:$(NC)"
	@echo "  make                    # Build release version"
	@echo "  make debug              # Build debug version"
	@echo "  make run                # Build and run"
	@echo "  make BUILD_TYPE=Debug   # Build with specific type"
	@echo "  make clean build        # Clean rebuild"

# Initial setup - fetch dependencies
setup: fetch-deps
	@echo "$(GREEN)Setup complete! Run 'make build' to compile.$(NC)"

# Fetch dependencies using CMake
fetch-deps:
	@echo "$(BLUE)Fetching dependencies...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -DUSE_LOCAL_FTXUI=OFF $(CMAKE_OPTIONS)
	@echo "$(GREEN)Dependencies fetched successfully!$(NC)"

# Build target
build:
	@echo "$(BLUE)Building Base OS TUI ($(BUILD_TYPE))...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. $(CMAKE_OPTIONS)
	@cd $(BUILD_DIR) && cmake --build . -j$(JOBS)
	@echo "$(GREEN)Build complete!$(NC)"
	@echo "$(YELLOW)Executable: $(BUILD_DIR)/base_os_tui$(NC)"

# Debug build
debug:
	@$(MAKE) build BUILD_TYPE=Debug

# Release build
release:
	@$(MAKE) build BUILD_TYPE=Release

# Clean build directory
clean:
	@echo "$(YELLOW)Cleaning build directory...$(NC)"
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)Clean complete!$(NC)"

# Run the application
run: build
	@echo "$(BLUE)Running Base OS TUI...$(NC)"
	@echo "$(YELLOW)================================$(NC)"
	@$(BUILD_DIR)/base_os_tui

# Install the application
install: release
	@echo "$(BLUE)Installing to $(PREFIX)/bin...$(NC)"
	@cd $(BUILD_DIR) && cmake --install . --prefix $(PREFIX)
	@echo "$(GREEN)Installation complete!$(NC)"

# Uninstall the application
uninstall:
	@echo "$(YELLOW)Uninstalling from $(PREFIX)/bin...$(NC)"
	@rm -f $(PREFIX)/bin/base_os_tui
	@echo "$(GREEN)Uninstall complete!$(NC)"

# Run tests (placeholder for future tests)
test: build
	@echo "$(BLUE)Running tests...$(NC)"
	@if [ -f $(BUILD_DIR)/tests/base_os_tui_tests ]; then \
		$(BUILD_DIR)/tests/base_os_tui_tests; \
	else \
		echo "$(YELLOW)No tests found. Tests will be added in future updates.$(NC)"; \
	fi

# Quick rebuild (clean and build)
rebuild: clean build

# Development build with all warnings
dev:
	@$(MAKE) build BUILD_TYPE=Debug CMAKE_OPTIONS="$(CMAKE_OPTIONS) -DCMAKE_CXX_FLAGS='-Wall -Wextra -Wpedantic -Werror'"

# Static analysis (requires clang-tidy)
analyze:
	@echo "$(BLUE)Running static analysis...$(NC)"
	@if command -v clang-tidy >/dev/null 2>&1; then \
		find src -name "*.cpp" -exec clang-tidy {} -p=$(BUILD_DIR) \; ; \
	else \
		echo "$(YELLOW)clang-tidy not found. Install it for static analysis.$(NC)"; \
	fi

# Format code (requires clang-format)
format:
	@echo "$(BLUE)Formatting code...$(NC)"
	@if command -v clang-format >/dev/null 2>&1; then \
		find src include -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs clang-format -i; \
		echo "$(GREEN)Code formatted!$(NC)"; \
	else \
		echo "$(YELLOW)clang-format not found. Install it for code formatting.$(NC)"; \
	fi

# Check if dependencies are available
check-deps:
	@echo "$(BLUE)Checking dependencies...$(NC)"
	@command -v cmake >/dev/null 2>&1 || { echo "$(RED)CMake not found! Please install CMake 3.18+$(NC)"; exit 1; }
	@command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 || { echo "$(RED)C++ compiler not found!$(NC)"; exit 1; }
	@echo "$(GREEN)All required dependencies found!$(NC)"

# Docker build (for consistent environment)
docker-build:
	@echo "$(BLUE)Building in Docker container...$(NC)"
	@docker build -t base-os-tui .
	@docker run --rm -v $(PWD):/workspace base-os-tui make build

# Create source distribution
dist:
	@echo "$(BLUE)Creating source distribution...$(NC)"
	@git archive --format=tar.gz --prefix=base-os-tui/ HEAD > base-os-tui-$(shell date +%Y%m%d).tar.gz
	@echo "$(GREEN)Distribution created: base-os-tui-$(shell date +%Y%m%d).tar.gz$(NC)"

# Show build configuration
info:
	@echo "$(BLUE)Build Configuration:$(NC)"
	@echo "  CMake version: $(shell cmake --version | head -1)"
	@echo "  Compiler: $(shell $(CXX) --version | head -1)"
	@echo "  Build type: $(BUILD_TYPE)"
	@echo "  Build directory: $(BUILD_DIR)"
	@echo "  Install prefix: $(PREFIX)"
	@echo "  Parallel jobs: $(JOBS)"

.DEFAULT_GOAL := help
