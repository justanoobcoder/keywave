CXX = g++

VERSION    := $(shell cat VERSION 2>/dev/null || echo "0.0.0")
GIT_COMMIT ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_DATE := $(shell date -u +"%Y-%m-%dT%H:%M:%SZ")

CXXFLAGS = -O2 -std=c++17 -Wall -Wextra
LDFLAGS = -lpthread -lm
INCLUDES = -Iinclude

VERSION_H = include/keywave/version.h

SRCS = src/audio.cpp \
       src/config.cpp \
       src/device.cpp \
       src/soundpack.cpp

OBJS = $(SRCS:.cpp=.o)

TARGET = keywave
TEST_TARGET  = test_keywave
TEST_SRCS    = tests/test_main.cpp \
               tests/test_config.cpp \
               tests/test_soundpack.cpp \
               tests/test_audio.cpp \
               tests/test_device.cpp
TEST_OBJS    = $(TEST_SRCS:.cpp=.o)

PREFIX   ?= /usr
BINDIR   ?= $(PREFIX)/bin
MANDIR   ?= $(PREFIX)/share/man
MAN1DIR  ?= $(MANDIR)/man1
MAN5DIR  ?= $(MANDIR)/man5

all: $(TARGET)

$(VERSION_H): VERSION
	@mkdir -p include/keywave
	@echo "/* auto-generated, do not edit */" > $@
	@echo "#pragma once" >> $@
	@echo "#include <string_view>" >> $@
	@echo "namespace keywave {" >> $@
	@echo "inline constexpr std::string_view APP_VERSION = \"$(VERSION)\";" >> $@
	@echo "inline constexpr std::string_view GIT_COMMIT  = \"$(GIT_COMMIT)\";" >> $@
	@echo "inline constexpr std::string_view BUILD_DATE  = \"$(BUILD_DATE)\";" >> $@
	@echo "}" >> $@

$(TARGET): $(OBJS) src/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

%.o: %.cpp $(VERSION_H)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f src/*.o tests/*.o $(TARGET) $(TEST_TARGET) $(VERSION_H)

install: $(TARGET)
	install -D -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -D -m 644 docs/man/keywave.1 $(DESTDIR)$(MAN1DIR)/keywave.1
	install -D -m 644 docs/man/keywave.conf.5 $(DESTDIR)$(MAN5DIR)/keywave.conf.5

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(MAN1DIR)/keywave.1
	rm -f $(DESTDIR)$(MAN5DIR)/keywave.conf.5

.PHONY: all clean install uninstall test
