#include "sprite.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include <SDL_image.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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


Sprite::Sprite(const std::vector<unsigned char>& imageData)
    : textureID(0), VAO(0), VBO(0), EBO(0),
      width(0), height(0), posX(0), posY(0), rotation(0.0f),
      r(1.0f), g(1.0f), b(1.0f), a(1.0f)
{
    if (!loadTextureFromMemory(imageData)) {
        std::cerr << "Failed to load sprite texture." << std::endl;
    }
    initRenderData();
}

Sprite::~Sprite() {
    glDeleteTextures(1, &textureID);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}


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


void Sprite::initRenderData() {
    // Define vertex positions and texture coordinates for a rectangle (quad)
    float vertices[] = {
        // Position   // Texture coordinates
        0.0f, 1.0f,   0.0f, 1.0f,  // bottom-left
        1.0f, 1.0f,   1.0f, 1.0f,  // bottom-right
        1.0f, 0.0f,   1.0f, 0.0f,  // top-right
        0.0f, 0.0f,   0.0f, 0.0f   // top-left
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
      // Position (2 components)
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
      // Texture coordinates (2 components)
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
      glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

//
// Rendering
//
void Sprite::draw() {
    // Render using internal position and angle
    draw(Vector2(static_cast<float>(posX), static_cast<float>(posY)), rotation);
}

void Sprite::draw(const Vector2& pos, float angle) {
    glDisable(GL_DEPTH_TEST);
    // Update position and angle
    posX = static_cast<int>(pos.x);
    posY = static_cast<int>(pos.y);
    rotation = angle;
    //glDisable(GL_DEPTH_TEST);
    GLuint prog = ResourceManager::Get().GetOrCreateShader("sprite", vertexShaderSource, fragmentShaderSource);
    glUseProgram(prog);

    // Build model matrix: translate, rotate around center, and scale
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
    model = glm::translate(model, glm::vec3(width * 0.5f, height * 0.5f, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-width * 0.5f, -height * 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));

    // Create orthographic projection
    // Window assumed to be 800x480; adjust as needed.
    glm::mat4 projection = Renderer::Get().GetOrthoProjection();
    
    GLint modelLoc = glGetUniformLocation(prog, "model");
    GLint projLoc  = glGetUniformLocation(prog, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Set sprite color and opacity
    GLint colorLoc = glGetUniformLocation(prog, "spriteColor");
    glUniform4f(colorLoc, r, g, b, a);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLint texLoc = glGetUniformLocation(prog, "spriteTexture");
    glUniform1i(texLoc, 0);

    // Draw quad
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

//
// Setters and getters
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
