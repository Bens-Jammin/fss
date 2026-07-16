# Name of your final program
TARGET = fss.exe
# Static library that the Rust frontend links against
LIB = libfssbackend.a
# Rust
RUST_BIN_NAME = fss_cli
RUST_DIR = src/cli
CARGO = cargo

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Isrc
LDLIBS = -lsqlite3

BUILD_DIR = build

SRC_CPPS = $(wildcard src/*.cpp)
APP_OBJS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC_CPPS))
LIB_OBJS = $(filter-out $(BUILD_DIR)/main.o, $(APP_OBJS))

TEST_SRCS = $(wildcard tests/*.cpp)

TARGET_PATH = $(BUILD_DIR)/$(TARGET)
LIB_PATH = $(BUILD_DIR)/$(LIB)
TEST_RUNNER_PATH = $(BUILD_DIR)/test_runner.exe

ifeq ($(OS),Windows_NT)
    RUN_CMD = .\$(TARGET_PATH)
    RUST_BIN = $(RUST_DIR)\target\release\$(RUST_BIN_NAME).exe
else
    RUN_CMD = ./$(TARGET_PATH)
    RUST_BIN = $(RUST_DIR)/target/release/$(RUST_BIN_NAME)
endif

CLEAN_CMD = rm -rf $(BUILD_DIR) nul

.PHONY: all build test run clean lib rust stack run-stack help

all: build test run

build: $(TARGET_PATH)

$(TARGET_PATH): $(APP_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(APP_OBJS) -o $@ $(LDLIBS)

test: build $(TEST_RUNNER_PATH)
	./$(TEST_RUNNER_PATH) -s

$(TEST_RUNNER_PATH): $(LIB_OBJS) $(TEST_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LIB_OBJS) $(TEST_SRCS) -o $@ $(LDLIBS)

run: build
	$(RUN_CMD)

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

lib: $(LIB_PATH)

$(LIB_PATH): $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)
	ar rcs $@ $(LIB_OBJS)

rust: $(LIB_PATH)
	cd $(RUST_DIR) && $(CARGO) build --release

stack: rust

run-stack: stack
	$(RUST_BIN)

clean:
	$(CLEAN_CMD)
	cd $(RUST_DIR) && $(CARGO) clean

help:
	@echo "make            - build, test, run C++ standalone (default)"
	@echo "make build      - build C++ standalone binary only"
	@echo "make lib        - build libfssbackend.a only"
	@echo "make stack      - build backend lib + Rust frontend"
	@echo "make run-stack  - build and run the full Rust+C++ stack"
	@echo "make clean      - clean C++ and Rust build artifacts"