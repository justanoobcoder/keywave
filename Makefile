CXX := g++
CXXFLAGS := -std=c++17 -lpthread -lm -I./include

TARGET := keywave
SRC := src/*.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
