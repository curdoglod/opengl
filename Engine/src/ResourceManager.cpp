#include "ResourceManager.h"
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>

ResourceManager& ResourceManager::Get() {
    static ResourceManager instance;
    return instance;
}

// ── Textures ──────────────────────────────────────────────────────────────────

GLuint ResourceManager::LoadTexture(const std::string& path) {
    auto it = m_textureCache.find(path);
    if (it != m_textureCache.end()) return it->second;

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "ResourceManager: cannot load '" << path
                  << "': " << IMG_GetError() << std::endl;
        return 0;
    }
    GLuint id = uploadSurface(surface);
    SDL_FreeSurface(surface);
    m_textureCache[path] = id;
    return id;
}

GLuint ResourceManager::LoadTextureFromMemory(const std::string& key,
                                               const std::vector<unsigned char>& data) {
    auto it = m_textureCache.find(key);
    if (it != m_textureCache.end()) return it->second;

    SDL_RWops* rw = SDL_RWFromConstMem(data.data(), (int)data.size());
    if (!rw) return 0;
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) {
        std::cerr << "ResourceManager: cannot load from memory '" << key
                  << "': " << SDL_GetError() << std::endl;
        return 0;
    }
    GLuint id = uploadSurface(surface);
    SDL_FreeSurface(surface);
    m_textureCache[key] = id;
    return id;
}

GLuint ResourceManager::uploadSurface(SDL_Surface* surface) {
    SDL_Surface* conv = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    if (!conv) return 0;

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 conv->w, conv->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, conv->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    SDL_FreeSurface(conv);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

// ── Shaders ───────────────────────────────────────────────────────────────────

GLuint ResourceManager::GetOrCreateShader(const std::string& name,
                                           const char* vertSrc,
                                           const char* fragSrc) {
    auto it = m_shaderCache.find(name);
    if (it != m_shaderCache.end()) return it->second;

    GLuint program = compileProgram(vertSrc, fragSrc);
    m_shaderCache[name] = program;
    return program;
}

static GLuint compileShader_impl(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "ResourceManager shader compile error: " << log << std::endl;
    }
    return shader;
}

GLuint ResourceManager::compileProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vs   = compileShader_impl(GL_VERTEX_SHADER, vertSrc);
    GLuint fs   = compileShader_impl(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "ResourceManager shader link error: " << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void ResourceManager::ReleaseAll() {
    for (auto& [key, id] : m_textureCache)
        glDeleteTextures(1, &id);
    m_textureCache.clear();

    for (auto& [name, prog] : m_shaderCache)
        glDeleteProgram(prog);
    m_shaderCache.clear();
}
