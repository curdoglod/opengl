#ifndef MODEL3D_COMPONENT_H
#define MODEL3D_COMPONENT_H

#include "component.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

// Вспомогательные структуры
struct MeshEntry {
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    unsigned int numIndices;
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

    // Можно добавить сеттеры/геттеры для материала, шейдера и т.д.

private:
    bool loadModel(const std::string& path);
    void processNode(struct aiNode* node, const struct aiScene* scene);
    void processMesh(struct aiMesh* mesh, const struct aiScene* scene);

    // Создаём шейдерную программу
    static GLuint shaderProgram;
    static GLuint loadShaderProgram();

private:
    std::string modelPath;
    std::vector<MeshEntry> meshes;

    // Параметры для трансформации/камеры (можно расширить)
    // Вы можете использовать шейдер, как в Sprite/TextComponent,
    // или иметь отдельные шейдеры для 3D
};

#endif // MODEL3D_COMPONENT_H
