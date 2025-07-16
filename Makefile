CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

LUA_CFLAGS := -I/usr/include/lua5.4
LUA_LDFLAGS := -L/usr/lib -llua5.4 -ldl -pthread

SOURCES := main.cpp editor.cpp input.cpp highlight.cpp plugin.cpp
HEADERS := editor.h input.h highlight.h plugin.h
OBJECTS := $(SOURCES:.cpp=.o)
TARGET := azyon

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) $(LUA_LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LUA_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

rebuild: clean all