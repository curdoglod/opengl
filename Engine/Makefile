UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    CC = clang++
    CFLAGS = -std=c++17 -Iinclude -I/opt/homebrew/include `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf SDL2_image glew assimp`
    LDFLAGS = `sdl2-config --libs` `pkg-config --libs SDL2_ttf SDL2_image glew assimp`
else
    CC = g++
    CFLAGS = -std=c++17 -DGLEW_STATIC -Iinclude `sdl2-config --cflags` `pkg-config --cflags SDL2_ttf SDL2_image glew assimp`
    LDFLAGS = `sdl2-config --libs` `pkg-config --libs SDL2_ttf SDL2_image glew assimp` -lGLEW -lGL
endif

LIB_NAME = libEngine.a
SRC = $(wildcard src/*.cpp)
OBJDIR = obj
OBJ = $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRC))
AR = ar rcs

.PHONY: all clean

all: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	$(AR) $@ $^

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJDIR)/*.o $(LIB_NAME)
	rm -rf $(OBJDIR)
