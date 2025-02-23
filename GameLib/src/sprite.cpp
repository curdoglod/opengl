#include "sprite.h"
#include <SDL_image.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Инициализация статического члена (шейдерная программа)
GLuint Sprite::shaderProgram = 0;

//
// Шейдеры (вершинный и фрагментный)
//
static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 projection;

out vec2 TexCoord;

void main()
{
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D spriteTexture;
uniform vec4 spriteColor;

void main()
{
    FragColor = texture(spriteTexture, TexCoord) * spriteColor;
}
)";

// Вспомогательная функция для компиляции шейдера
static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Ошибка компиляции шейдера: " << infoLog << std::endl;
    }
    return shader;
}

// Загружает и линкует шейдерную программу
GLuint Sprite::loadShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Ошибка линковки шейдерной программы: " << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

//
// Конструктор и деструктор
//
Sprite::Sprite(const std::vector<unsigned char>& imageData)
    : textureID(0), VAO(0), VBO(0), EBO(0),
      width(0), height(0), posX(0), posY(0), rotation(0.0f),
      r(1.0f), g(1.0f), b(1.0f), a(1.0f)
{
    if (!loadTextureFromMemory(imageData)) {
        std::cerr << "Не удалось загрузить текстуру спрайта." << std::endl;
    }
    initRenderData();
    if (shaderProgram == 0) {
        shaderProgram = loadShaderProgram();
    }
}

Sprite::~Sprite() {
    glDeleteTextures(1, &textureID);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

//
// Загрузка текстуры
//
bool Sprite::loadTextureFromMemory(const std::vector<unsigned char>& imageData) {
    SDL_RWops* rw = SDL_RWFromConstMem(imageData.data(), imageData.size());
    if (!rw) {
        std::cerr << "SDL_RWFromConstMem failed: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) {
        std::cerr << "IMG_Load_RW failed: " << SDL_GetError() << std::endl;
        return false;
    }
    GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    width = surface->w;
    height = surface->h;

    SDL_FreeSurface(surface);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

//
// Инициализация геометрии спрайта (прямоугольника)
//
void Sprite::initRenderData() {
    // Определяем координаты вершин и текстурные координаты для прямоугольника (quad)
    float vertices[] = {
        // Позиция    // Текстурные координаты
        0.0f, 1.0f,   0.0f, 1.0f,  // Нижний левый
        1.0f, 1.0f,   1.0f, 1.0f,  // Нижний правый
        1.0f, 0.0f,   1.0f, 0.0f,  // Верхний правый
        0.0f, 0.0f,   0.0f, 0.0f   // Верхний левый
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
      // Позиция (2 компоненты)
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
      // Текстурные координаты (2 компоненты)
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
      glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

//
// Отрисовка
//
void Sprite::draw() {
    // Отрисовка с использованием внутренних координат и угла
    draw(Vector2(static_cast<float>(posX), static_cast<float>(posY)), rotation);
}

void Sprite::draw(const Vector2& pos, float angle) {
    // Обновляем позицию и угол
    posX = static_cast<int>(pos.x);
    posY = static_cast<int>(pos.y);
    rotation = angle;

    glUseProgram(shaderProgram);

    // Создаём матрицу модели: перевод, поворот (вокруг центра) и масштабирование
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
    model = glm::translate(model, glm::vec3(width * 0.5f, height * 0.5f, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-width * 0.5f, -height * 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));

    // Создаём ортографическую проекцию
    // Здесь предполагается, что окно имеет размер 1280x720. При необходимости можно передавать реальные размеры.
    glm::mat4 projection = glm::ortho(0.0f, 800.0f, 480.0f, 0.0f, -1.0f, 1.0f);
    
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint projLoc  = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Устанавливаем цвет и прозрачность спрайта
    GLint colorLoc = glGetUniformLocation(shaderProgram, "spriteColor");
    glUniform4f(colorLoc, r, g, b, a);

    // Привязываем текстуру
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLint texLoc = glGetUniformLocation(shaderProgram, "spriteTexture");
    glUniform1i(texLoc, 0);

    // Отрисовываем прямоугольник
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

//
// Сеттеры и геттеры
//
void Sprite::setPosition(int x, int y) {
    posX = x;
    posY = y;
}

void Sprite::setAngle(float angle) {
    rotation = angle;
}

void Sprite::setSize(int w, int h) {
    width = w;
    height = h;
}

Vector2 Sprite::getSize() const {
    return Vector2(width, height);
}

void Sprite::SetColorAndOpacity(Uint8 red, Uint8 green, Uint8 blue, float alpha) {
    r = red / 255.0f;
    g = green / 255.0f;
    b = blue / 255.0f;
    a = alpha;
}
