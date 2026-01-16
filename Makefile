CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -I/usr/include/eigen3 -I/usr/local/include
LDFLAGS = -lssl -lcrypto
SRC = src/test_db.cpp
TARGET = test_db

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
