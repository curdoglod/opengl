#include "text.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "engine.h" // Предполагается, что здесь есть GetDefaultArchive()
#include "Utils.h"  // Если нужен ваш Vector2, Color и т.д.

// ============================ Шейдеры ============================
static const char *vertexShaderSource = R"(
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

static const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec4 textColor;

void main()
{
    // Берём цвет пикселя из текстуры
    vec4 sampled = texture(textTexture, TexCoord);
    // Умножаем на textColor (можно регулировать прозрачность и т.д.)
    FragColor = sampled * textColor;
}
)";

// Статический член класса
GLuint TextComponent::shaderProgram = 0;

// Вспомогательная функция для компиляции шейдера
static GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Ошибка компиляции шейдера: " << infoLog << std::endl;
    }
    return shader;
}

// Линкует шейдерную программу
GLuint TextComponent::loadShaderProgram()
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Ошибка линковки шейдерной программы: " << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// ============================ Конструктор/деструктор ============================
TextComponent::TextComponent(int fontSize, const std::string &text, const Color &color, TextAlignment align)
    : fontSize(fontSize), text(text), color(color), alignment(align), font(nullptr), textureID(0), textWidth(0), textHeight(0), VAO(0), VBO(0), EBO(0)
{
}
TextComponent::TextComponent(int fontSize, const std::string &text, TextAlignment align)
    : fontSize(fontSize), text(text), color(Color(255, 255, 255, 255)), alignment(align), font(nullptr), textureID(0), textWidth(0), textHeight(0), VAO(0), VBO(0), EBO(0)
{
}
TextComponent::~TextComponent()
{
    if (textureID)
    {
        glDeleteTextures(1, &textureID);
    }
    if (VAO)
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    if (font)
    {
        TTF_CloseFont(font);
    }
}

// ============================ Инициализация ============================
void TextComponent::Init()
{
    // Загрузка шрифта из DefaultArchive (по аналогии с вашим кодом)
    fontDataBuffer = Engine::GetDefaultArchive()->GetFile("Roboto-Black.ttf");
    SDL_RWops *rw = SDL_RWFromConstMem(fontDataBuffer.data(), fontDataBuffer.size());
    if (!rw)
    {
        std::cerr << "Не удалось создать SDL_RWops для шрифта" << std::endl;
        return;
    }
    font = TTF_OpenFontRW(rw, 1, fontSize);
    if (!font)
    {
        std::cerr << "Не удалось открыть шрифт: " << TTF_GetError() << std::endl;
        return;
    }

    // Создаём шейдерную программу (единожды, если 0)
    if (shaderProgram == 0)
    {
        shaderProgram = loadShaderProgram();
    }

    // Создаём VAO/VBO/EBO для отрисовки (квадрат 1x1, потом масштабируем)
    initRenderData();

    // Создаём текстуру из текста
    updateTexture();
}

void TextComponent::initRenderData()
{
    // Простой квадрат (0,0) -> (1,1), потом будем масштабировать под текст
    float vertices[] = {
        // Позиция  // Текстурные координаты
        0.0f, 1.0f, 0.0f, 1.0f, // левый нижний
        1.0f, 1.0f, 1.0f, 1.0f, // правый нижний
        1.0f, 0.0f, 1.0f, 0.0f, // правый верхний
        0.0f, 0.0f, 0.0f, 0.0f  // левый верхний
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0};

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Позиция
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Текстурные координаты
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ============================ Создание текстуры из SDL_Surface ============================
bool TextComponent::createTextureFromSurface(SDL_Surface *surface)
{
    if (!surface)
        return false;

    // Если уже есть старая текстура — удалим
    if (textureID)
    {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    textWidth = surface->w;
    textHeight = surface->h;

    // Определяем формат (часто это RGBA, если TTF_RenderText_Solid => формат может быть 8bit+colormap)
    // Лучше использовать TTF_RenderText_Blended, чтобы получить 32bpp (RGBA)
    GLenum format = GL_RGBA;
    int bytesPerPixel = surface->format->BytesPerPixel;
    if (bytesPerPixel == 4)
    {
        // Предположим RGBA
        format = GL_RGBA;
    }
    else
    {
        // Предположим RGB
        format = GL_RGB;
    }

    // Создаём OpenGL-текстуру
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Параметры текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Загрузка пикселей в текстуру
    glTexImage2D(GL_TEXTURE_2D, 0, format, textWidth, textHeight, 0, format,
                 GL_UNSIGNED_BYTE, surface->pixels);

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

// ============================ Пересоздание текста ============================
void TextComponent::updateTexture()
{
    if (!font)
        return;

    // SDL_ttf иногда возвращает surface с форматом 8bpp (Indexed), лучше использовать Blended
    SDL_Color sdlColor = {color.r, color.g, color.b, color.a};
    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), sdlColor);
    if (!surface)
    {
        std::cerr << "Не удалось создать Surface из текста: " << TTF_GetError() << std::endl;
        return;
    }

    // Преобразуем в формат RGBA32 (Byte order: RGBA)
    SDL_Surface *convSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface); // Исходная поверхность больше не нужна

    if (!convSurface)
    {
        std::cerr << "SDL_ConvertSurfaceFormat failed: " << SDL_GetError() << std::endl;
        return;
    }

    // Теперь convSurface->format->BytesPerPixel == 4, и порядок каналов RGBA
    // Можно безопасно загрузить в OpenGL:
    createTextureFromSurface(convSurface);

    SDL_FreeSurface(convSurface);
}

// ============================ Публичные методы ============================
void TextComponent::setText(const std::string &newText)
{
    text = newText;
    updateTexture();
}

void TextComponent::setColor(const Color &newColor)
{
    color = newColor;
    updateTexture();
}

void TextComponent::setAlignment(TextAlignment newAlignment)
{
    alignment = newAlignment;
}

// ============================ Update (рендер) ============================
void TextComponent::Update(float dt)
{
    // Если нет текстуры или VAO — нечего рисовать
    if (!textureID || !VAO)
        return;

    // Для простоты возьмём позицию и угол из object (как у вас в коде)
    float angle = object->GetAngle();
    Vector2 pos = object->GetPosition();
    Vector2 size = object->GetSize(); // Обычно вы используете это для фона и т.д.

    // Считаем модельную матрицу
    glm::mat4 model(1.0f);

    // 1. Перенос в позицию
    // Выравнивание по X:
    switch (alignment)
    {
    case TextAlignment::LEFT:
        // ничего дополнительно
        break;
    case TextAlignment::CENTER:
        pos.x += (size.x * 0.5f) - (textWidth * 0.5f);
        break;
    case TextAlignment::RIGHT:
        pos.x += (size.x) - textWidth;
        break;
    }
    // По Y пусть будет по центру объекта
    pos.y += (size.y * 0.5f) - (textHeight * 0.5f);

    // Переносим в итоговую позицию
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));

    // 2. Если хотите вращать текст вокруг его центра:
    model = glm::translate(model, glm::vec3(textWidth * 0.5f, textHeight * 0.5f, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 0, 1));
    model = glm::translate(model, glm::vec3(-textWidth * 0.5f, -textHeight * 0.5f, 0.0f));

    // 3. Масштабируем под размер текста
    model = glm::scale(model, glm::vec3(textWidth, textHeight, 1.0f));

    // Ортографическая проекция (пример)
    // Зависит от того, какую вы используете:
    glm::mat4 projection = glm::ortho(0.0f, 800.0f, 480.0f, 0.0f, -1.0f, 1.0f);

    // Рендер
    glUseProgram(shaderProgram);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Цвет (если хотим динамически менять)
    GLint colorLoc = glGetUniformLocation(shaderProgram, "textColor");
    glUniform4f(colorLoc, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

    // Привязываем текстуру
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLint texLoc = glGetUniformLocation(shaderProgram, "textTexture");
    glUniform1i(texLoc, 0);

    // Рисуем quad
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Отключаемся
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}
