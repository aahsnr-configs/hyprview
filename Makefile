PLUGIN_NAME = hyprview

CXX = g++
CXXFLAGS = -std=c++23 -fPIC -Wall -Wextra -O2 -DNDEBUG -Isrc
CXXFLAGS += $(shell pkg-config --cflags hyprland)
LDFLAGS = -shared
LDLIBS = $(shell pkg-config --libs hyprland)

SOURCES = src/main.cpp src/Hyprview.cpp src/GestureManager.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = $(PLUGIN_NAME).so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	@echo "Use 'hyprpm add .' to install, or copy $(TARGET) manually."

.PHONY: all clean install
