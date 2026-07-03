# Name of your final program
TARGET = fss.exe

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Isrc
LDLIBS = -lsqlite3

APP_OBJS = $(filter-out src/main.o, $(patsubst src/%.cpp,src/%.o,$(wildcard src/*.cpp)))
TEST_SRCS = $(wildcard tests/*.cpp)

ifeq ($(OS),Windows_NT)
    RUN_CMD = .\$(TARGET)
else
    RUN_CMD = ./$(TARGET)
endif


ifeq ($(OS),Windows_NT)
	CLEAN_CMD = del $(TARGET)
else
	CLEAN_CMD = rm -f $(TARGET)
endif


# Compile everything in src/ at once
all: build test run

clean:
	$(CLEAN_CMD)

build:
	$(CXX) $(CXXFLAGS) src/*.cpp -o $(TARGET) $(LDLIBS)

test:
	$(CXX) $(CXXFLAGS) $(filter-out src/main.cpp, $(wildcard src/*.cpp)) $(TEST_SRCS) -o test_runner.exe $(LDLIBS)
	./test_runner.exe -s

run: build
	$(RUN_CMD)

.PHONY: test clean all run