# Name of your final program
TARGET = program.exe

CXX = g++
CXXFLAGS = -std=c++20 -O2 -Ithird_party/BTree/include

# Compile everything in src/ at once
all:
	$(CXX) $(CXXFLAGS) src/*.cpp -o $(TARGET)

# Delete the program to start fresh
clean:
	del $(TARGET)
