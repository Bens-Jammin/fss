# Name of your final program
TARGET = fss.exe
# Static library that the Rust frontend links against
LIB = libfssbackend.a

# Rust workspace
APPS_DIR = apps
TUI_BIN_NAME = fss_tui
CLI_BIN_NAME = fss_cli
CARGO = cargo

CPP_DIR   = engine/src
TESTS_DIR = engine/tests

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -I$(CPP_DIR)
LDLIBS = -lsqlite3

BUILD_DIR = build

SRC_CPPS = $(wildcard $(CPP_DIR)/*.cpp)
APP_OBJS = $(patsubst $(CPP_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_CPPS))
LIB_OBJS = $(filter-out $(BUILD_DIR)/main.o, $(APP_OBJS))

TEST_SRCS = $(wildcard $(TESTS_DIR)/*.cpp)

TARGET_PATH      = $(BUILD_DIR)/$(TARGET)
LIB_PATH         = $(BUILD_DIR)/$(LIB)
TEST_RUNNER_PATH = $(BUILD_DIR)/test_runner.exe

ifeq ($(OS),Windows_NT)
    RUN_CMD  = .\$(TARGET_PATH)
    TUI_BIN  = $(APPS_DIR)\target\release\$(TUI_BIN_NAME).exe
    CLI_BIN  = $(APPS_DIR)\target\release\$(CLI_BIN_NAME).exe
else
    RUN_CMD  = ./$(TARGET_PATH)
    TUI_BIN  = $(APPS_DIR)/target/release/$(TUI_BIN_NAME)
    CLI_BIN  = $(APPS_DIR)/target/release/$(CLI_BIN_NAME)
endif

CLEAN_CMD = rm -rf $(BUILD_DIR)

.PHONY: all build test run clean lib rust-tui rust-cli tui cli run-tui run-cli help

all: build test run

build: $(TARGET_PATH)

$(TARGET_PATH): $(APP_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(APP_OBJS) -o $@ $(LDLIBS)

test: $(TEST_RUNNER_PATH)
	./$(TEST_RUNNER_PATH) -s

$(TEST_RUNNER_PATH): $(LIB_OBJS) $(TEST_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(TESTS_DIR) $(LIB_OBJS) $(TEST_SRCS) -o $@ $(LDLIBS)

run: build
	$(RUN_CMD)

$(BUILD_DIR)/%.o: $(CPP_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

lib: $(LIB_PATH)

$(LIB_PATH): $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)
	ar rcs $@ $(LIB_OBJS)

# Build individual frontends (sys build.rs pulls in the C++ lib itself)
rust-tui: lib
	cd $(APPS_DIR) && $(CARGO) build --release -p $(TUI_BIN_NAME)

rust-cli: lib
	cd $(APPS_DIR) && $(CARGO) build --release -p $(CLI_BIN_NAME)

tui: rust-tui
run-tui: tui
	$(TUI_BIN)

cli: rust-cli
run-cli: cli
	$(CLI_BIN)

clean:
	$(CLEAN_CMD)
	cd $(APPS_DIR) && $(CARGO) clean

help:
	@echo "make            - build, test, run C++ standalone (default)"
	@echo "make build      - build C++ standalone binary only"
	@echo "make test       - build and run the doctest test runner"
	@echo "make lib        - build libfssbackend.a only"
	@echo "make tui        - build the Rust TUI frontend"
	@echo "make run-tui    - build and run the Rust TUI frontend"
	@echo "make cli        - build the Rust CLI frontend"
	@echo "make run-cli    - build and run the Rust CLI frontend"
	@echo "make clean      - clean C++ and Rust build artifacts"