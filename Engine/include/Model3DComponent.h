#pragma once
#include "component.h"
#include "ResourceManager.h"
#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <limits>

class Model3DComponent : public Component
{
public:
    Model3DComponent(const std::string& modelPath);
    virtual ~Model3DComponent();

    virtual void Init() override;

    // Interpret Object::size as scale factors relative to imported dimensions
    void SetSizeIsRelative(bool enabled) { sizeIsRelative = enabled; }
    bool GetSizeIsRelative() const { return sizeIsRelative; }

    // Added: expose model import-space AABB and computed dimensions
    bool HasAabb() const { return aabbComputed; }
    glm::vec3 GetAabbMin() const { return aabbMin; }
    glm::vec3 GetAabbMax() const { return aabbMax; }
    glm::vec3 GetModelDims() const { return modelDims; }

    // Helpers for lighting/shadows
    glm::mat4 ComputeModelMatrix() const;
    void RenderDepthPass(const glm::mat4& model, GLuint depthProgram) const;

    /// Called by RenderSystem — renders the model with the given camera and light.
    void Render(const glm::mat4& view, const glm::mat4& projection, class LightComponent* light);

    // Override albedo/diffuse texture from code
    void SetAlbedoTexture(GLuint textureId) { overrideAlbedoTexture = textureId; }
    bool SetAlbedoTextureFromFile(const std::string& fullPath);

    // Highlight overlay — call SetHighlight(true) to tint, SetHighlight(false) to clear
    void SetHighlight(bool enabled,
                      glm::vec3 color     = glm::vec3(1.0f, 1.0f, 0.4f),
                      float     intensity = 0.04f)
    {
        highlightTint = enabled ? glm::vec4(color, intensity) : glm::vec4(0.0f);
    }

private:
    std::string modelPath;
    // Shared geometry from ResourceManager (parsed once, used by all instances)
    const SharedMeshData* sharedMesh = nullptr;

    // Axis-aligned bounding box of the imported model in model space
    glm::vec3 aabbMin = glm::vec3( std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
    bool aabbComputed = false;
    glm::vec3 modelDims = glm::vec3(0.0f);
    bool sizeIsRelative = true;

    // Optional override for albedo texture
    GLuint overrideAlbedoTexture = 0;

    // Highlight/tint overlay (rgb = color, a = mix factor)
    glm::vec4 highlightTint = glm::vec4(0.0f);
};
