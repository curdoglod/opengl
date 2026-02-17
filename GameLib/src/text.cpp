#include "text.h"
#include "Renderer.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "engine.h" // Assumes GetDefaultArchive() is available here
#include "Utils.h"  // If you need your Vector2, Color, etc.

// ============================ Shaders ============================
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
    // Sample pixel color from texture
    vec4 sampled = texture(textTexture, TexCoord);
    // Multiply by textColor (can control opacity, etc.)
    FragColor = sampled * textColor;
}
)";

// Static class member
GLuint TextComponent::shaderProgram = 0;

// Helper function to compile shader
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
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
    }
    return shader;
}

// Link shader program
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
        std::cerr << "Shader program link error: " << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// ============================ Constructor/Destructor ============================
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

// ============================ Initialization ============================
void TextComponent::Init()
{
    // Load font from DefaultArchive
    fontDataBuffer = Engine::GetDefaultArchive()->GetFile("Roboto-Black.ttf");
    SDL_RWops *rw = SDL_RWFromConstMem(fontDataBuffer.data(), fontDataBuffer.size());
    if (!rw)
    {
        std::cerr << "Failed to create SDL_RWops for font" << std::endl;
        return;
    }
    font = TTF_OpenFontRW(rw, 1, fontSize);
    if (!font)
    {
        std::cerr << "Failed to open font: " << TTF_GetError() << std::endl;
        return;
    }

    // Create shader program (once, if 0)
    if (shaderProgram == 0)
    {
        shaderProgram = loadShaderProgram();
    }

    // Create VAO/VBO/EBO for rendering (1x1 quad, then scale)
    initRenderData();

    // Create texture from text
    updateTexture();
}

void TextComponent::initRenderData()
{
    // Simple quad (0,0) -> (1,1), later scaled to text size
    float vertices[] = {
        // Position  // TexCoords
        0.0f, 1.0f, 0.0f, 1.0f, // bottom-left
        1.0f, 1.0f, 1.0f, 1.0f, // bottom-right
        1.0f, 0.0f, 1.0f, 0.0f, // top-right
        0.0f, 0.0f, 0.0f, 0.0f  // top-left
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

    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ============================ Create OpenGL texture from SDL_Surface ============================
bool TextComponent::createTextureFromSurface(SDL_Surface *surface)
{
    if (!surface)
        return false;

    // Delete old texture if present
    if (textureID)
    {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    textWidth = surface->w;
    textHeight = surface->h;

    // Determine format (prefer TTF_RenderText_Blended to get 32bpp RGBA)
    GLenum format = GL_RGBA;
    int bytesPerPixel = surface->format->BytesPerPixel;
    if (bytesPerPixel == 4)
    {
        // Assume RGBA
        format = GL_RGBA;
    }
    else
    {
        // Assume RGB
        format = GL_RGB;
    }

    // Create OpenGL texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels to texture
    glTexImage2D(GL_TEXTURE_2D, 0, format, textWidth, textHeight, 0, format,
                 GL_UNSIGNED_BYTE, surface->pixels);

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

// ============================ Recreate text texture ============================
void TextComponent::updateTexture()
{
    if (!font)
        return;

    // SDL_ttf may return 8bpp surface; use Blended to ensure RGBA32
    SDL_Color sdlColor = {color.r, color.g, color.b, color.a};
    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), sdlColor);
    if (!surface)
    {
        std::cerr << "Failed to create text surface: " << TTF_GetError() << std::endl;
        return;
    }

    // Convert to RGBA32 (Byte order: RGBA)
    SDL_Surface *convSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface); // Original surface no longer needed

    if (!convSurface)
    {
        std::cerr << "SDL_ConvertSurfaceFormat failed: " << SDL_GetError() << std::endl;
        return;
    }

    // Now convSurface->format->BytesPerPixel == 4 with RGBA channel order
    // Safe to upload to OpenGL:
    createTextureFromSurface(convSurface);

    SDL_FreeSurface(convSurface);
}

// ============================ Public methods ============================
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

// ============================ Update (render) ============================
void TextComponent::Update(float dt)
{
    (void)dt;
    // Logic only — rendering moved to LateUpdate
}

void TextComponent::LateUpdate(float dt)
{
    glDisable(GL_DEPTH_TEST);
    // Nothing to draw if texture or VAO is missing
    if (!textureID || !VAO)
        return;

    // For simplicity, take position and angle from object
    float angle = object->GetAngle().z;
    Vector2 pos = object->GetPosition();
    Vector2 size = object->GetSize(); // Commonly used for background etc.

    // Compute model matrix
    glm::mat4 model(1.0f);

    // 1. Translation to position
    // Horizontal alignment:
    switch (alignment)
    {
    case TextAlignment::LEFT:
        // nothing extra
        break;
    case TextAlignment::CENTER:
        pos.x += (size.x * 0.5f) - (textWidth * 0.5f);
        break;
    case TextAlignment::RIGHT:
        pos.x += (size.x) - textWidth;
        break;
    }
    // Vertically center within object
    pos.y += (size.y * 0.5f) - (textHeight * 0.5f);

    // Apply final translation
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));

    // 2. Rotate text around its center
    model = glm::translate(model, glm::vec3(textWidth * 0.5f, textHeight * 0.5f, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 0, 1));
    model = glm::translate(model, glm::vec3(-textWidth * 0.5f, -textHeight * 0.5f, 0.0f));

    // 3. Scale to text size
    model = glm::scale(model, glm::vec3(textWidth, textHeight, 1.0f));

    // Orthographic projection (example)
    // Depends on what you use:
    glm::mat4 projection = glm::ortho(0.0f, 800.0f, 480.0f, 0.0f, -1.0f, 1.0f);

    // Render
    glUseProgram(shaderProgram);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Color (if changing dynamically)
    GLint colorLoc = glGetUniformLocation(shaderProgram, "textColor");
    glUniform4f(colorLoc, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLint texLoc = glGetUniformLocation(shaderProgram, "textTexture");
    glUniform1i(texLoc, 0);

    // Draw quad
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Cleanup
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}
