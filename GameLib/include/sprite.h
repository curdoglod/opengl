#ifndef SPRITE_H
#define SPRITE_H

#include <vector>
#include <SDL.h>
#include <GL/glew.h>
#include "Utils.h"  
#include <glm/glm.hpp>

class Sprite {
public:
  
    Sprite(const std::vector<unsigned char>& imageData);
    ~Sprite();

    void draw();
    void draw(const Vector2& pos, float angle);

    void setPosition(int x, int y);
    void setAngle(float angle);
    void setSize(int width, int height);
    Vector2 getSize() const;
    void SetColorAndOpacity(Uint8 red, Uint8 green, Uint8 blue, float alpha);

private:
    bool loadTextureFromMemory(const std::vector<unsigned char>& imageData);
    void initRenderData();

    GLuint textureID;
    GLuint VAO, VBO, EBO;
    int width, height;
    int posX, posY;
    float rotation;
    float r, g, b, a;


};

#endif // SPRITE_H
