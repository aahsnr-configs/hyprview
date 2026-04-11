# Makefile for hyprview plugin on Fedora

PLUGIN_NAME = hyprview
HYPRLAND_SOURCE = ./Hyprland  # Path to the cloned Hyprland source

CXX = g++
CXXFLAGS = -std=c++23 -fPIC -Wall -Wextra -O2 -Isrc \
           -I$(HYPRLAND_SOURCE) \
           -I$(HYPRLAND_SOURCE)/subprojects/wlroots/include \
           -I$(HYPRLAND_SOURCE)/protocols \
           $(shell pkg-config --cflags pixman-1 libdrm wayland-server xcb xcb-util libinput libudev)
LDFLAGS = -shared
LDLIBS = $(shell pkg-config --libs pixman-1 libdrm wayland-server)

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

.PHONY: all clean
