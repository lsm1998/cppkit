# ===========================================
#  Generic Makefile for CMake-based projects
#  Auto-detects executable name after build
# ===========================================

BUILD_DIR   := build
PID_FILE    := $(BUILD_DIR)/app.pid
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug

# 自动检测可执行文件：取 build/ 下最新的可执行文件（排除 CMake 自己的文件）
EXECUTABLE := $(shell find $(BUILD_DIR) -type f -executable ! -name "cmake*" 2>/dev/null | head -n 1)

# 默认目标
.PHONY: all
all: build

# --- 构建 ---
.PHONY: build
build:
	@echo "🔧 Building project with CMake..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) .. -DENABLE_TESTING=ON
	@cmake --build $(BUILD_DIR) -- -j$$(nproc)
	@echo "✅ Build complete."
	@echo "➡️  Executables found:"
	@find $(BUILD_DIR) -maxdepth 1 -type f -executable ! -name "cmake*" 2>/dev/null || echo "  (none)"
	@echo ""

# --- 启动程序 ---
.PHONY: start
start:
	@if [ -z "$(EXECUTABLE)" ]; then \
		echo "❌ No executable found in $(BUILD_DIR). Run 'make build' first."; \
		exit 1; \
	fi
	@if [ -f "$(PID_FILE)" ]; then \
		echo "⚠️  Process already running (PID: $$(cat $(PID_FILE)))."; \
		exit 0; \
	fi
	@echo "🚀 Starting program: $(EXECUTABLE)"
	@nohup $(EXECUTABLE) > $(BUILD_DIR)/app.log 2>&1 & echo $$! > $(PID_FILE)
	@echo "✅ Started with PID: $$(cat $(PID_FILE))"

# --- 停止程序 ---
.PHONY: stop
stop:
	@if [ -f "$(PID_FILE)" ]; then \
		PID=$$(cat $(PID_FILE)); \
		echo "🛑 Stopping process (PID: $$PID)..."; \
		kill $$PID || true; \
		rm -f $(PID_FILE); \
		echo "✅ Stopped."; \
	else \
		echo "⚠️  No running process found."; \
	fi

# --- 重启 ---
.PHONY: restart
restart: stop start

# --- 清理 ---
.PHONY: clean
clean:
	@echo "🧹 Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "✅ Clean complete."

# --- 帮助 ---
.PHONY: help
help:
	@echo ""
	@echo "Usage:"
	@echo "  make build     - Build project using CMake"
	@echo "  make start     - Start the built executable (auto-detected)"
	@echo "  make stop      - Stop running executable"
	@echo "  make restart   - Restart program"
	@echo "  make clean     - Remove build directory"
	@echo ""
