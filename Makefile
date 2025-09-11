UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    CC = clang++
    CFLAGS = -std=c++17 -Iinclude -IGameLib/include -I/opt/homebrew/include `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf SDL2_image glew assimp`
    LIBS = -LGameLib -lGameLib -L/opt/homebrew/lib `sdl2-config --libs` `pkg-config --libs SDL2_ttf SDL2_image glew assimp` -framework OpenGL
else
    CC = g++
    CFLAGS = -std=c++17 -Iinclude -IGameLib/include `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf SDL2_image assimp`
    LIBS = -LGameLib -lGameLib `sdl2-config --libs` `pkg-config --libs SDL2_ttf SDL2_image assimp` -lGLEW -lGL
endif

TARGET = game
LIB_NAME = GameLib/libGameLib.a

SRC = $(wildcard src/*.cpp)
OBJDIR = obj
OBJ = $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRC))

AR = ar rcs

.PHONY: all clean clean_lib clean_all re re_fast re_full

all: $(LIB_NAME) $(TARGET)

$(LIB_NAME):
	cd GameLib && $(MAKE)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LIBS)

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)


clean:
	rm -f $(OBJDIR)/*.o $(TARGET)
	rm -rf $(OBJDIR)

clean_lib:
	cd GameLib && $(MAKE) clean

clean_all: clean clean_lib

re_full: clean_all all

re_fast: clean $(TARGET)

re: re_full
