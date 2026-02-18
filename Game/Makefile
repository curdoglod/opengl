UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    CC = clang++
    CFLAGS = -std=c++17 -Iinclude -I../Engine/include -I/opt/homebrew/include `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf SDL2_image glew assimp`
    LIBS = -L../Engine -lEngine -L/opt/homebrew/lib `sdl2-config --libs` `pkg-config --libs SDL2_ttf SDL2_image glew assimp` -framework OpenGL
else
    CC = g++
    CFLAGS = -std=c++17 -Iinclude -I../Engine/include `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf SDL2_image assimp`
    LIBS = -L../Engine -lEngine `sdl2-config --libs` `pkg-config --libs SDL2_ttf SDL2_image assimp` -lGLEW -lGL
endif

TARGET = ../game_app
ENGINE_LIB = ../Engine/libEngine.a

SRC = $(wildcard src/*.cpp)
OBJDIR = obj
OBJ = $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRC))

.PHONY: all clean engine

all: engine $(TARGET)

engine:
	cd ../Engine && $(MAKE)

$(TARGET): $(OBJ) $(ENGINE_LIB)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LIBS)

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJDIR)/*.o $(TARGET)
	rm -rf $(OBJDIR)
