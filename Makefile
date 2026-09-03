CXX = g++

CXXFLAGS = -O2 -std=c++17 -Wall
LDFLAGS = -lpthread -lm
INCLUDES = -Iinclude

SRCS = src/audio.cpp \
       src/device.cpp

OBJS = $(SRCS:.cpp=.o)

TARGET = keywave

PREFIX  ?= /usr
BINDIR  ?= $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(OBJS) src/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f src/*.o *.o $(TARGET)

install: $(TARGET)
	install -D -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

.PHONY: all clean install uninstall
