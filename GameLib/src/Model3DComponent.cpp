#include "Model3DComponent.h"
#include "engine.h"  // Если нужно читать файл из архива, или получить WindowSize
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// ==================== Шейдеры для упрощённой отрисовки ====================
static const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
// Можно добавить нормали, текстурные координаты

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    // Простой белый цвет (можно сделать uniform для цвета/текстуры)
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

// Статическая переменная для шейдера
GLuint Model3DComponent::shaderProgram = 0;

// Вспомогательная функция компиляции шейдера
static GLuint compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Ошибка компиляции шейдера: " << infoLog << std::endl;
    }
    return shader;
}

// Линкуем шейдерную программу
GLuint Model3DComponent::loadShaderProgram()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Ошибка линковки шейдерной программы: " << infoLog << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// ==================== Конструктор/Деструктор ====================
Model3DComponent::Model3DComponent(const std::string& modelPath)
    : modelPath(modelPath)
{
}

Model3DComponent::~Model3DComponent()
{
    // Удаляем все VAO/VBO/EBO
    for (auto& mesh : meshes) {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
    }
}

// ==================== Init: Загрузка модели + настройка шейдера ====================
void Model3DComponent::Init()
{
    // Загружаем модель
    if (!loadModel(modelPath)) {
        std::cerr << "Не удалось загрузить модель: " << modelPath << std::endl;
    }

    // Если шейдер ещё не загружен, загружаем
    if (shaderProgram == 0) {
        shaderProgram = loadShaderProgram();
    }
}

// ==================== Update: Рендер модели ====================
void Model3DComponent::Update(float dt)
{
   // glEnable(GL_DEPTH_TEST); 
    // Допустим, мы хотим позиционировать 3D-модель по данным из object
    glm::vec3 position(object->GetPosition3D().x, object->GetPosition3D().y, object->GetPosition3D().z);
    Vector3 angle = object->GetAngle();
    // object->GetSize() — можно интерпретировать как масштаб?

    // 1. Матрица модели (translate -> rotate -> scale)
    glm::mat4 model = glm::mat4(1.0f);
    // Перенос
    model = glm::translate(model, position);
    // Поворот вокруг оси Z (для 2D), можно расширить
    model = glm::rotate(model, glm::radians(angle.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(angle.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(angle.z), glm::vec3(0, 0, 1));    // Масштаб
    Vector2 size = object->GetSize();
    model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

    // 2. Матрица вида (камера). Упрощённо — смотрим из (0,0,5) в (0,0,0).
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // 3. Матрица проекции (перспектива). Можно взять из ваших настроек.
    // Если окно 800x480, к примеру:
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        800.0f / 480.0f,  // соотношение сторон
        0.1f, 
        100.0f
    );

    // Рендер
    glUseProgram(shaderProgram);

    // Передаём матрицы в шейдер
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc  = glGetUniformLocation(shaderProgram,  "view");
    GLint projLoc  = glGetUniformLocation(shaderProgram,  "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc,  1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc,  1, GL_FALSE, glm::value_ptr(projection));

    // Отрисовываем каждый mesh
    for (auto& mesh : meshes) {
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glUseProgram(0);
}

// ==================== Загрузка модели через Assimp ====================
bool Model3DComponent::loadModel(const std::string& path)
{
    Assimp::Importer importer;
    // postprocess: Triangulate — разбить полигоны на треугольники
    //             FlipUVs    — если нужно перевернуть UV
    //             ... см. документацию
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenNormals  |
        aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    // Рекурсивно обходим узлы сцены
    processNode(scene->mRootNode, scene);

    return true;
}

void Model3DComponent::processNode(aiNode* node, const aiScene* scene)
{
    // Обрабатываем все меши, связанные с этим узлом
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }
    // Рекурсивно обходим дочерние узлы
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void Model3DComponent::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<float> vertices;  // позиция + ... (если нужно)
    std::vector<unsigned int> indices;

    // Собираем вершины
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // Позиция
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
        // Если нужны нормали:
        // if (mesh->HasNormals()) {
        //     vertices.push_back(mesh->mNormals[i].x);
        //     vertices.push_back(mesh->mNormals[i].y);
        //     vertices.push_back(mesh->mNormals[i].z);
        // }
        // Если нужны UV:
        // if (mesh->mTextureCoords[0]) {
        //     vertices.push_back(mesh->mTextureCoords[0][i].x);
        //     vertices.push_back(mesh->mTextureCoords[0][i].y);
        // } else {
        //     vertices.push_back(0.0f);
        //     vertices.push_back(0.0f);
        // }
    }

    // Собираем индексы
    for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
        aiFace face = mesh->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Создаём VAO/VBO/EBO для этого меша
    MeshEntry entry;
    glGenVertexArrays(1, &entry.VAO);
    glGenBuffers(1, &entry.VBO);
    glGenBuffers(1, &entry.EBO);

    glBindVertexArray(entry.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, entry.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entry.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // Допустим, у нас только позиция (3 float'а)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    entry.numIndices = (unsigned int)indices.size();
    meshes.push_back(entry);
}
