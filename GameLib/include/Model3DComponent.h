#pragma once
#include "component.h"
#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <assimp/scene.h>

// Структура для хранения текстуры
struct Texture {
    GLuint id;
    std::string type;
    std::string path;
};

// Для каждого меша
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
    // Конструктор принимает путь к файлу модели (FBX, OBJ, etc.)
    Model3DComponent(const std::string& modelPath);
    virtual ~Model3DComponent();

    // Переопределяем методы Component
    virtual void Init() override;   // Загрузка модели (через Assimp)
    virtual void Update(float dt) override; // Рендер модели

private:
    bool loadModel(const std::string& path);
    void processNode(struct aiNode* node, const struct aiScene* scene);
    void processMesh(struct aiMesh* mesh, const struct aiScene* scene);
    std::vector<Texture> loadMaterialTextures(struct aiMaterial* mat, aiTextureType type, const std::string& typeName);
    GLuint TextureFromFile(const char* path, const std::string& directory);

    // Создаём шейдерную программу
    static GLuint shaderProgram;
    static GLuint loadShaderProgram();

private:
    std::string modelPath;
    std::string directory; // директория модели
    std::vector<MeshEntry> meshes;
    std::vector<Texture> loadedTextures; // для предотвращения повторной загрузки текстур
};

