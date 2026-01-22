# ══════════════════════════════════════════════════════════════════════════════
#                              SPEECHER Makefile
# ══════════════════════════════════════════════════════════════════════════════

# ─────────────────────────────────────────────────────────────────────────────
#                              Configuration
# ─────────────────────────────────────────────────────────────────────────────

PROJECT      = speecher
VERSION      = 0.5.0

SRC_DIR      = src
INC_DIR      = include
BUILD_DIR    = build
OBJ_DIR_UNIX = $(BUILD_DIR)/obj_unix
OBJ_DIR_WIN  = $(BUILD_DIR)/obj_win

# Compilers
CC_UNIX      = gcc
CC_WIN       = x86_64-w64-mingw32-gcc

# Targets
TARGET_UNIX  = $(BUILD_DIR)/$(PROJECT)
TARGET_WIN   = $(BUILD_DIR)/$(PROJECT).exe

# Flags
CFLAGS_BASE  = -std=c99 -Wall -Wextra -Wpedantic -I$(INC_DIR) -O2 -DNDEBUG
CFLAGS_UNIX  = $(CFLAGS_BASE) -DLINUX
CFLAGS_WIN   = $(CFLAGS_BASE) -DWINDOWS

LDFLAGS_UNIX = -lm
LDFLAGS_WIN  = -lws2_32 -static

# Sources
SRCS         = $(shell find $(SRC_DIR) -name '*.c')
OBJS_UNIX    = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_UNIX)/%.o,$(SRCS))
OBJS_WIN     = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_WIN)/%.o,$(SRCS))

# ─────────────────────────────────────────────────────────────────────────────
#                              Main Targets
# ─────────────────────────────────────────────────────────────────────────────

.PHONY: all dual unix win clean help setup status unsetup

# Default
all: help

# Build for both systems
dual: unix win
	@echo ""
	@echo "══════════════════════════════════════════════════════════"
	@echo "  ✓ Dual build complete!"
	@echo ""
	@echo "  Unix:    $(TARGET_UNIX)  ($$(du -h $(TARGET_UNIX) | cut -f1))"
	@echo "  Windows: $(TARGET_WIN)   ($$(du -h $(TARGET_WIN) | cut -f1))"
	@echo "══════════════════════════════════════════════════════════"

# Unix build (Linux/macOS)
unix: $(TARGET_UNIX)
	@echo ""
	@echo "══════════════════════════════════════════════════════════"
	@echo "  ✓ Unix build complete: $(TARGET_UNIX)"
	@echo "  Size: $$(du -h $(TARGET_UNIX) | cut -f1)"
	@echo "══════════════════════════════════════════════════════════"

# Windows build (cross-compile)
win: check-mingw $(TARGET_WIN)
	@echo ""
	@echo "══════════════════════════════════════════════════════════"
	@echo "  ✓ Windows build complete: $(TARGET_WIN)"
	@echo "  Size: $$(du -h $(TARGET_WIN) | cut -f1)"
	@echo "══════════════════════════════════════════════════════════"

# ─────────────────────────────────────────────────────────────────────────────
#                              Build Rules
# ─────────────────────────────────────────────────────────────────────────────

# Link Unix executable
$(TARGET_UNIX): $(OBJS_UNIX)
	@mkdir -p $(BUILD_DIR)
	@echo "Linking Unix executable..."
	@$(CC_UNIX) $(OBJS_UNIX) -o $@ $(LDFLAGS_UNIX)

# Compile Unix object files
$(OBJ_DIR_UNIX)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $< (Unix)..."
	@$(CC_UNIX) $(CFLAGS_UNIX) -c $< -o $@

# Link Windows executable
$(TARGET_WIN): $(OBJS_WIN)
	@mkdir -p $(BUILD_DIR)
	@echo "Linking Windows executable..."
	@$(CC_WIN) $(OBJS_WIN) -o $@ $(LDFLAGS_WIN)

# Compile Windows object files
$(OBJ_DIR_WIN)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $< (Windows)..."
	@$(CC_WIN) $(CFLAGS_WIN) -c $< -o $@

# ─────────────────────────────────────────────────────────────────────────────
#                              Clean
# ─────────────────────────────────────────────────────────────────────────────

clean:
	@echo "Cleaning project..."
	@rm -rf $(BUILD_DIR)
	@find $(SRC_DIR) -name '*.o' -delete 2>/dev/null || true
	@find $(SRC_DIR) -name '*.d' -delete 2>/dev/null || true
	@find . -name '*~' -delete 2>/dev/null || true
	@find . -name '*.swp' -delete 2>/dev/null || true
	@find . -name '.DS_Store' -delete 2>/dev/null || true
	@find . -name '*.orig' -delete 2>/dev/null || true
	@find . -name '*.rej' -delete 2>/dev/null || true
	@echo "✓ Project cleaned"

# ─────────────────────────────────────────────────────────────────────────────
#                              Developer Tools
# ─────────────────────────────────────────────────────────────────────────────

# Check if MinGW cross-compiler is available
.PHONY: check-mingw
check-mingw:
	@which $(CC_WIN) > /dev/null 2>&1 || \
		(echo ""; \
		 echo "══════════════════════════════════════════════════════════"; \
		 echo "  ERROR: MinGW cross-compiler not found!"; \
		 echo ""; \
		 echo "  Run 'make setup' to install dependencies"; \
		 echo "══════════════════════════════════════════════════════════"; \
		 exit 1)

# Setup development environment
setup:
	@echo ""
	@echo "══════════════════════════════════════════════════════════"
	@echo "  Setting up development environment..."
	@echo "══════════════════════════════════════════════════════════"
	@echo ""
	@if [ -f /etc/debian_version ]; then \
		echo "Detected Debian/Ubuntu"; \
		echo "Installing: gcc, mingw-w64, make, git, vim..."; \
		sudo apt update && sudo apt install -y \
			gcc \
			make \
			mingw-w64 \
			git \
			vim \
			clang-format \
			valgrind \
			gdb; \
	elif [ -f /etc/arch-release ]; then \
		echo "Detected Arch Linux"; \
		echo "Installing: gcc, mingw-w64-gcc, make, git, vim..."; \
		sudo pacman -S --needed --noconfirm \
			gcc \
			make \
			mingw-w64-gcc \
			git \
			vim \
			clang \
			valgrind \
			gdb; \
	elif [ -f /etc/fedora-release ]; then \
		echo "Detected Fedora"; \
		echo "Installing: gcc, mingw64-gcc, make, git, vim..."; \
		sudo dnf install -y \
			gcc \
			make \
			mingw64-gcc \
			git \
			vim \
			clang-tools-extra \
			valgrind \
			gdb; \
	elif [ "$$(uname)" = "Darwin" ]; then \
		echo "Detected macOS"; \
		echo "Installing via Homebrew..."; \
		brew install \
			gcc \
			make \
			mingw-w64 \
			git \
			vim; \
	else \
		echo "Unknown OS. Please install manually:"; \
		echo "  - gcc"; \
		echo "  - make"; \
		echo "  - mingw-w64 (for Windows cross-compilation)"; \
		echo "  - git"; \
		echo "  - vim / editor of choice"; \
		echo "  - clang-format (optional, for code formatting)"; \
		echo "  - valgrind (optional, for memory debugging)"; \
		echo "  - gdb (optional, for debugging)"; \
		exit 1; \
	fi
	@echo ""
	@echo "✓ Setup complete! Run 'make status' to verify."

# Check development environment status
status:
	@echo ""
	@echo "══════════════════════════════════════════════════════════"
	@echo "  Development Environment Status"
	@echo "══════════════════════════════════════════════════════════"
	@echo ""
	@printf "  %-20s" "GCC (Unix):"
	@which $(CC_UNIX) > /dev/null 2>&1 && \
		echo "✓ $$($(CC_UNIX) --version | head -n1)" || \
		echo "✗ NOT FOUND"
	@printf "  %-20s" "MinGW (Windows):"
	@which $(CC_WIN) > /dev/null 2>&1 && \
		echo "✓ $$($(CC_WIN) --version | head -n1)" || \
		echo "✗ NOT FOUND"
	@printf "  %-20s" "Make:"
	@which make > /dev/null 2>&1 && \
		echo "✓ $$(make --version | head -n1)" || \
		echo "✗ NOT FOUND"
	@printf "  %-20s" "Git:"
	@which git > /dev/null 2>&1 && \
		echo "✓ $$(git --version)" || \
		echo "✗ NOT FOUND"
	@printf "  %-20s" "Clang-format:"
	@which clang-format > /dev/null 2>&1 && \
		echo "✓ $$(clang-format --version | head -n1)" || \
		echo "○ not installed (optional)"
	@printf "  %-20s" "Valgrind:"
	@which valgrind > /dev/null 2>&1 && \
		echo "✓ $$(valgrind --version)" || \
		echo "○ not installed (optional)"
	@printf "  %-20s" "GDB:"
	@which gdb > /dev/null 2>&1 && \
		echo "✓ $$(gdb --version | head -n1)" || \
		echo "○ not installed (optional)"
	@echo ""
	@echo "  Legend: ✓ installed  ✗ missing  ○ optional"
	@echo "══════════════════════════════════════════════════════════"

# Remove development tools (use with caution!)
unsetup:
	@echo ""
	@echo "══════════════════════════════════════════════════════════"
	@echo "  WARNING: This will remove development tools!"
	@echo "══════════════════════════════════════════════════════════"
	@echo ""
	@echo "This will remove:"
	@echo "  - mingw-w64 (Windows cross-compiler)"
	@echo "  - clang-format (code formatter)"
	@echo "  - valgrind (memory debugger)"
	@echo ""
	@echo "It will NOT remove: gcc, make, git, vim"
	@echo ""
	@read -p "Continue? [y/N] " -n 1 -r; \
	echo; \
	if [[ $$REPLY =~ ^[Yy]$$ ]]; then \
		if [ -f /etc/debian_version ]; then \
			sudo apt remove --purge -y mingw-w64 clang-format valgrind; \
		elif [ -f /etc/arch-release ]; then \
			sudo pacman -R --noconfirm mingw-w64-gcc clang valgrind; \
		elif [ -f /etc/fedora-release ]; then \
			sudo dnf remove -y mingw64-gcc clang-tools-extra valgrind; \
		elif [ "$$(uname)" = "Darwin" ]; then \
			brew uninstall mingw-w64; \
		fi; \
		echo "✓ Optional tools removed"; \
	else \
		echo "Cancelled"; \
	fi

# ─────────────────────────────────────────────────────────────────────────────
#                              Help
# ─────────────────────────────────────────────────────────────────────────────

help:
	@echo ""
	@echo "══════════════════════════════════════════════════════════"
	@echo "              SPEECHER Build System v$(VERSION)"
	@echo "══════════════════════════════════════════════════════════"
	@echo ""
	@echo "  Build Commands:"
	@echo "    make dual       Build for Unix and Windows"
	@echo "    make unix       Build for Linux/macOS"
	@echo "    make win        Build for Windows (cross-compile)"
	@echo "    make clean      Clean all build files"
	@echo ""
	@echo "  Developer Commands:"
	@echo "    make setup      Install development dependencies"
	@echo "    make status     Check development environment"
	@echo "    make unsetup    Remove optional dev tools"
	@echo "    make help       Show this help"
	@echo ""
	@echo "  Current System:"
	@echo "    OS:             $$(uname -s)"
	@echo "    Compiler:       $$($(CC_UNIX) --version 2>/dev/null | head -n1 || echo 'not found')"
	@echo "    MinGW:          $$($(CC_WIN) --version 2>/dev/null | head -n1 || echo 'not found')"
	@echo ""
	@echo "  Output:"
	@echo "    Unix binary:    $(TARGET_UNIX)"
	@echo "    Windows binary: $(TARGET_WIN)"
	@echo ""
	@echo "══════════════════════════════════════════════════════════"
	@echo ""