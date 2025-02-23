#ifndef SPRITE_H
#define SPRITE_H

#include <vector>
#include <SDL.h>
#include <GL/glew.h>
#include "Utils.h"  // Предполагается, что здесь определён класс Vector2
#include <glm/glm.hpp>

class Sprite {
public:
    // Конструктор создаёт спрайт из массива байтов с изображением
    Sprite(const std::vector<unsigned char>& imageData);
    ~Sprite();

    // Отрисовка спрайта (используются внутренние координаты)
    void draw();
    // Отрисовка спрайта по указанной позиции с поворотом
    void draw(const Vector2& pos, float angle);

    // Настройка параметров спрайта
    void setPosition(int x, int y);
    void setAngle(float angle);
    void setSize(int width, int height);
    Vector2 getSize() const;
    void SetColorAndOpacity(Uint8 red, Uint8 green, Uint8 blue, float alpha);

private:
    // Загружает текстуру из imageData
    bool loadTextureFromMemory(const std::vector<unsigned char>& imageData);
    // Инициализирует геометрию (VAO/VBO/EBO) для отрисовки квадрата
    void initRenderData();

    // OpenGL-идентификатор текстуры
    GLuint textureID;
    // VAO/VBO/EBO для отрисовки спрайта
    GLuint VAO, VBO, EBO;
    // Размеры текстуры и позиция спрайта
    int width, height;
    int posX, posY;
    float rotation;
    // Цвет спрайта (коэффициенты от 0.0 до 1.0)
    float r, g, b, a;

    // Общая шейдерная программа для отрисовки спрайтов
    static GLuint shaderProgram;
    // Загружает и компилирует шейдерную программу
    static GLuint loadShaderProgram();
};

#endif // SPRITE_H
