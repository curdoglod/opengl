#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <GL/glew.h>

struct SDL_Surface;  // forward declaration — avoid pulling in all of SDL.h

/// ResourceManager — central cache for GPU resources (textures + shaders).
///
/// All resources are indexed by a string key:
///   - textures from disk:   the full file path
///   - textures from memory: a user-supplied key (e.g. archive-relative path)
///   - shader programs:      a human-readable name (e.g. "sprite", "model3d")
///
/// The manager owns every GL object and keeps them alive until ReleaseAll()
/// is called (just before the GL context is destroyed).
class ResourceManager {
public:
    static ResourceManager& Get();

    // Non-copyable
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // ── Textures ─────────────────────────────────────────────────────────────

    /// Load a texture from a file path.  Returns the cached GLuint on repeat calls.
    GLuint LoadTexture(const std::string& path);

    /// Load a texture from in-memory image bytes.
    /// 'key' is a stable cache identifier (e.g. the archive-relative path).
    GLuint LoadTextureFromMemory(const std::string& key,
                                  const std::vector<unsigned char>& data);

    // ── Shaders ──────────────────────────────────────────────────────────────

    /// Return a compiled + linked shader program by name.
    /// If not yet compiled, builds it from the provided GLSL sources.
    GLuint GetOrCreateShader(const std::string& name,
                              const char* vertSrc,
                              const char* fragSrc);

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /// Delete all cached GL objects.  Call before the GL context is destroyed.
    void ReleaseAll();

private:
    ResourceManager() = default;

    GLuint uploadSurface(SDL_Surface* surface);
    static GLuint compileProgram(const char* vertSrc, const char* fragSrc);

    std::unordered_map<std::string, GLuint> m_textureCache;
    std::unordered_map<std::string, GLuint> m_shaderCache;
};

#endif // RESOURCE_MANAGER_H
