#pragma once
#include "component.h"
#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <limits>

struct Texture {
    GLuint id;
    std::string type;
    std::string path;
};

struct MeshEntry {
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    unsigned int numIndices;
    std::vector<Texture> textures;
};

class Model3DComponent : public Component
{
public:
    Model3DComponent(const std::string& modelPath);
    virtual ~Model3DComponent();

    virtual void Init() override;   
    virtual void Update(float dt) override;
    virtual void LateUpdate(float dt) override;

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

    // Override albedo/diffuse texture from code
    void SetAlbedoTexture(GLuint textureId) { overrideAlbedoTexture = textureId; }
    bool SetAlbedoTextureFromFile(const std::string& fullPath);

private:
    bool loadModel(const std::string& path);
    void processNode(struct aiNode* node, const struct aiScene* scene);
    void processMesh(struct aiMesh* mesh, const struct aiScene* scene);
    std::vector<Texture> loadMaterialTextures(struct aiMaterial* mat, aiTextureType type, const std::string& typeName);
    GLuint TextureFromFile(const char* path, const std::string& directory);


    std::string modelPath;
    std::string directory;
    std::vector<MeshEntry> meshes;

    // Axis-aligned bounding box of the imported model in model space
    glm::vec3 aabbMin = glm::vec3( std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
    bool aabbComputed = false;
    glm::vec3 modelDims = glm::vec3(0.0f);
    bool sizeIsRelative = true;

    // Optional override for albedo texture
    GLuint overrideAlbedoTexture = 0;
};

