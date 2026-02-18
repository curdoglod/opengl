#ifndef COLOR_H
#define COLOR_H

#include <SDL.h>
#include <cstdint>
#include <iostream>

class Color {
public:
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;

    Color() : r(0), g(0), b(0), a(255) {}
    Color(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
    Color(const SDL_Color& sdlColor)
        : r(sdlColor.r), g(sdlColor.g), b(sdlColor.b), a(sdlColor.a) {}

    operator SDL_Color() const {
        SDL_Color sdlColor = { r, g, b, a };
        return sdlColor;
    }

    void set(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255) {
        r = red;
        g = green;
        b = blue;
        a = alpha;
    }

    void print() const {
        std::cout << "Color(R: " << static_cast<int>(r)
                  << ", G: " << static_cast<int>(g)
                  << ", B: " << static_cast<int>(b)
                  << ", A: " << static_cast<int>(a) << ")" << std::endl;
    }
};

#endif // COLOR_H