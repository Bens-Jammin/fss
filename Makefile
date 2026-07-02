# Name of your final program
TARGET = fss.exe

CXX = g++
CXXFLAGS = -std=c++20 -O2
LDLIBS = -lsqlite3

ifeq ($(OS),Windows_NT)
    RUN_CMD = .\$(TARGET)
else
    RUN_CMD = ./$(TARGET)
endif


# Compile everything in src/ at once
all: build run

build:
	$(CXX) $(CXXFLAGS) src/*.cpp -o $(TARGET) $(LDLIBS)

clean:
ifeq ($(OS),Windows_NT)
	del $(TARGET)
else
	rm -f $(TARGET)
endif

run: build
	$(RUN_CMD)