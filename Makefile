CXX = g++
CXXFLAGS = $(shell pkg-config --cflags sdl3 sdl3-image)
LDFLAGS = $(shell pkg-config --libs sdl3 sdl3-image)

TARGET = main
SRC = src/main.cpp src/obj_parser.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) -Iinclude $(SRC) -o $(TARGET) $(CXXFLAGS) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)


compile_parser:
	$(CXX) -Iinclude -c src/obj_parser.cpp -o obj_parser.o
