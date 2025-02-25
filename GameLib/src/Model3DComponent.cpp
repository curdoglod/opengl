#include "Model3DComponent.h"
#include "engine.h"  // Для доступа к архивам, если требуется
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>

// ==================== Шейдеры (Lambert lighting) ====================
static const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPos;       // Позиция вершины
layout(location = 1) in vec2 aTexCoord;  // UV
layout(location = 2) in vec3 aNormal;    // Нормаль

out vec2 TexCoord;
out vec3 Normal;     
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Позиция вершины в мировых координатах
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position   = projection * view * worldPos;

    // Передаём в фрагментный шейдер
    FragPos  = worldPos.xyz;
    TexCoord = aTexCoord;

    // Приведение нормалей (учитывает масштаб/поворот)
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    Normal = normalize(normalMatrix * aNormal);
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

// Текстура модели
uniform sampler2D ourTexture;

// Параметры света
uniform vec3 lightDir;      // направление света
uniform vec3 lightColor;    // цвет света
uniform vec3 ambientColor;  // фоновое освещение

void main()
{
    // Цвет из текстуры
    vec3 texColor = texture(ourTexture, TexCoord).rgb;

    // Ламбертовская модель освещения
    vec3 norm = normalize(Normal);
    // lightDir считается направлением "от объекта к источнику" или наоборот,
    // поэтому иногда надо инвертировать знак. Ниже -dot, чтобы свет шёл "с камеры".
    float diff = max(dot(norm, -lightDir), 0.0);

    vec3 diffuse = diff * lightColor;
    vec3 ambient = ambientColor;

    // Итог
    vec3 result = texColor * (ambient + diffuse);
    FragColor   = vec4(result.rgb, 1.0);
    
}
)";

// ==================== Статическая переменная для шейдера ====================
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
    // Удаляем все VAO/VBO/EBO для каждого меша
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
    // Если шейдер ещё не загружен, загружаем его
    if (shaderProgram == 0) {
        shaderProgram = loadShaderProgram();
    }
}

// ==================== Update: Рендер модели ====================
void Model3DComponent::Update(float dt)
{
    glEnable(GL_DEPTH_TEST);
    // Получаем позицию, угол и масштаб из объекта
    glm::vec3 position(object->GetPosition3D().x/35, object->GetPosition3D().y/35, object->GetPosition3D().z/35);
    Vector3 angle = object->GetAngle(); // углы по x,y,z (в градусах)

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(angle.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(angle.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(angle.z), glm::vec3(0, 0, 1));

    // Масштабируем модель
    Vector2 size = object->GetSize();
    model = glm::scale(model, glm::vec3(size.x, size.y, size.x));

    // Матрица вида (камера)
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f),  // камеру можно настроить по желанию
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Перспективная проекция
    glm::mat4 projection = glm::perspective(
        glm::radians(90.0f),
        800.0f / 480.0f,
        0.1f,
        100.0f
    );

    glUseProgram(shaderProgram);

    // Передаём матрицы
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc  = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc  = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc,  1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc,  1, GL_FALSE, glm::value_ptr(projection));

    // Параметры освещения (примеры)
    GLint lightDirLoc     = glGetUniformLocation(shaderProgram, "lightDir");
    GLint lightColorLoc   = glGetUniformLocation(shaderProgram, "lightColor");
    GLint ambientColorLoc = glGetUniformLocation(shaderProgram, "ambientColor");
    // Допустим, свет летит по оси Z к объекту
    glUniform3f(lightDirLoc, 0.0f, 0.0f, -1.0f);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(ambientColorLoc, 0.2f, 0.2f, 0.2f);

    // Рендерим каждый меш
    for (auto& mesh : meshes) {
        if (!mesh.textures.empty()) {
            // Привязываем первую текстуру
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textures[0].id);
            glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
        }
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
    // Вместо aiProcess_GenNormals используем aiProcess_GenSmoothNormals
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate  |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    // Определяем директорию модели (для поиска текстур)
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
    return true;
}

void Model3DComponent::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void Model3DComponent::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    bool hasTexCoords = mesh->HasTextureCoords(0);
    bool hasNormals   = mesh->HasNormals(); // Должно быть true, т.к. мы используем GenSmoothNormals

    // Порядок: (позиция x,y,z) + (нормаль x,y,z) + (UV x,y)
    // Итого 8 float на вершину
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // Позиция
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);

        // Нормаль (если нет, подставим (0,0,1))
        if (hasNormals) {
            vertices.push_back(mesh->mNormals[i].x);
            vertices.push_back(mesh->mNormals[i].y);
            vertices.push_back(mesh->mNormals[i].z);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
        }

        // UV
        if (hasTexCoords) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    // Индексы
    for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
        aiFace face = mesh->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    MeshEntry entry;
    glGenVertexArrays(1, &entry.VAO);
    glGenBuffers(1, &entry.VBO);
    glGenBuffers(1, &entry.EBO);

    glBindVertexArray(entry.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, entry.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entry.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Всего 8 float на вершину: (pos.x,pos.y,pos.z, norm.x,norm.y,norm.z, uv.x,uv.y)
    int stride = 8 * sizeof(float);

    // layout(location=0) -> aPos (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // layout(location=2) -> aNormal (vec3), идёт после 3 float позиции
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // layout(location=1) -> aTexCoord (vec2), идёт после 6 float (pos+normal)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    entry.numIndices = static_cast<unsigned int>(indices.size());
    
    // Загружаем диффузные текстуры (или baseColor, если нужно)
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        std::cout << "Material index " << mesh->mMaterialIndex
                  << " has " << material->GetTextureCount(aiTextureType_DIFFUSE)
                  << " diffuse textures\n";
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        entry.textures.insert(entry.textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    }

    meshes.push_back(entry);
}

std::vector<Texture> Model3DComponent::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for (unsigned int j = 0; j < loadedTextures.size(); j++) {
            if (std::strcmp(loadedTextures[j].path.data(), str.C_Str()) == 0) {
                textures.push_back(loadedTextures[j]);
                skip = true;
                break;
            }
        }
        if (!skip) {
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            loadedTextures.push_back(texture);
        }
    }
    return textures;
}

GLuint Model3DComponent::TextureFromFile(const char* path, const std::string& directory)
{
    std::string filename = std::string(path);
    filename = directory + "/" + filename;

    SDL_Surface* surface = IMG_Load(filename.c_str());
    if (!surface) {
        std::cerr << "Не удалось загрузить текстуру: " << IMG_GetError() << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Настройка параметров текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_FreeSurface(surface);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}
