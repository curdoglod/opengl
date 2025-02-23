UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    CC = clang++
    CFLAGS = -std=c++17 -Iinclude -IGameLib/include -I/opt/homebrew/include `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf SDL2_image`
    LIBS = -LGameLib -lGameLib -L/opt/homebrew/lib `sdl2-config --libs` `pkg-config --libs SDL2_ttf SDL2_image`
else
    CC = g++
    CFLAGS = -std=c++17 -Iinclude -IGameLib/include `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf SDL2_image`
    LIBS = -LGameLib -lGameLib `sdl2-config --libs` `pkg-config --libs SDL2_ttf SDL2_image` -lGLEW -lGL
endif

TARGET = game
LIB_NAME = GameLib/libGameLib.a

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)

AR = ar rcs

.PHONY: all clean re

all: $(LIB_NAME) $(TARGET)

$(LIB_NAME):
	cd GameLib && $(MAKE)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)
	cd GameLib && $(MAKE) clean

re: clean all
