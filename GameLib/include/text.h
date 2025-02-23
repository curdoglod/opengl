#ifndef TEXT_GL_H
#define TEXT_GL_H

#include "component.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "color.h"

// Простейшее выравнивание
enum class TextAlignment {
    LEFT,
    CENTER,
    RIGHT
};

class TextComponent : public Component
{
public:
    TextComponent(int fontSize,
           const std::string& text,
           const Color& color  = Color(255, 255, 255, 255),
           TextAlignment align = TextAlignment::LEFT);
    TextComponent(int fontSize,
           const std::string& text,
           TextAlignment align = TextAlignment::LEFT);
    ~TextComponent();

    void setText(const std::string& newText);
    void setColor(const Color& newColor);
    void setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) { setColor(Color(r, g, b, a)); }
    void setAlignment(TextAlignment newAlignment);

    virtual void Init() override;       // Инициализация (загрузка шрифта, создание текстуры)
    virtual void Update(float dt) override; // Рендер текста

    // Клонирование компонента (если нужно)
    virtual TextComponent* Clone() const override {
        return new TextComponent(fontSize, text, color, alignment);
    }

private:
    bool createTextureFromSurface(SDL_Surface* surface);
    void updateTexture();     // Пересоздаёт текстуру из текущего текста
    void initRenderData();    // Создаёт VAO/VBO/EBO для рендеринга

    // Шейдерная программа (можно сделать общую для спрайтов/текста)
    static GLuint shaderProgram;
    static GLuint loadShaderProgram();

private:
    int fontSize;
    std::string text;
    Color color;
    TextAlignment alignment;

    // Данные о шрифте
    TTF_Font* font;
    std::vector<unsigned char> fontDataBuffer;

    // Данные о созданной текстуре
    GLuint textureID;
    int textWidth;
    int textHeight;

    // VAO/VBO/EBO для рендеринга квадрата
    GLuint VAO, VBO, EBO;
};

#endif // TEXT_GL_H
