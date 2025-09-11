#include "Model3DComponent.h"
#include "engine.h"  // Access to archives if needed
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include "CameraComponent.h"

// ==================== Shaders (Lambert lighting) ====================
static const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPos;       // Vertex position
layout(location = 1) in vec2 aTexCoord;  // UV
layout(location = 2) in vec3 aNormal;    // Normal

out vec2 TexCoord;
out vec3 Normal;     
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Vertex position in world coordinates
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position   = projection * view * worldPos;

    // Pass to fragment shader
    FragPos  = worldPos.xyz;
    TexCoord = aTexCoord;

    // Normal transform (accounts for scale/rotation)
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

// Model texture
uniform sampler2D ourTexture;

// Light parameters
uniform vec3 lightDir;      // light direction
uniform vec3 lightColor;    // light color
uniform vec3 ambientColor;  // ambient color

void main()
{
    // Texture color
    vec3 texColor = texture(ourTexture, TexCoord).rgb;

    // Lambert lighting model
    vec3 norm = normalize(Normal);
    // Depending on convention, lightDir may require sign inversion. Using -dot below.
    float diff = max(dot(norm, -lightDir), 0.0);

    vec3 diffuse = diff * lightColor;
    vec3 ambient = ambientColor;

    // Final color
    vec3 result = texColor * (ambient + diffuse);
    FragColor   = vec4(result.rgb, 1.0);
    
}
)";

// ==================== Static shader variable ====================
GLuint Model3DComponent::shaderProgram = 0;

// Helper to compile shader
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
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
    }
    return shader;
}

// Link shader program
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
        std::cerr << "Shader program link error: " << infoLog << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// ==================== Constructor/Destructor ====================
Model3DComponent::Model3DComponent(const std::string& modelPath)
    : modelPath(modelPath)
{
}

Model3DComponent::~Model3DComponent()
{
    // Delete all VAO/VBO/EBO for each mesh
    for (auto& mesh : meshes) {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
    }
}

// ==================== Init: Load model + setup shader ====================
void Model3DComponent::Init()
{
    // Load model
    if (!loadModel(modelPath)) {
        std::cerr << "Failed to load model: " << modelPath << std::endl;
    }
    // Load shader if not yet loaded
    if (shaderProgram == 0) {
        shaderProgram = loadShaderProgram();
    }
    // If object size not set by user, default to native Blender-imported dimensions
    if (aabbComputed) {
        if (object && object->GetSize3D().x == 0 && object->GetSize3D().y == 0 && object->GetSize3D().z == 0) {
            if (modelDims == glm::vec3(0.0f)) {
                modelDims = aabbMax - aabbMin;
            }
            object->SetSize(Vector3(modelDims.x, modelDims.y, modelDims.z));
        }
    }
}

// ==================== Update: Render model ====================
void Model3DComponent::Update(float dt)
{
    glEnable(GL_DEPTH_TEST);
    // Get position, rotation and scale from object
    glm::vec3 position(object->GetPosition3D().x/35, object->GetPosition3D().y/35, object->GetPosition3D().z/35);
    Vector3 angle = object->GetAngle(); // angles x,y,z (degrees)

    // Build model matrix with correct pivot and order
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 local = glm::mat4(1.0f);

    // Fit AABB to object size around its center in local space
    Vector3 targetSize = object->GetSize3D();
    // If size is treated as relative scale factors, convert to absolute target size
    if (sizeIsRelative && aabbComputed) {
        glm::vec3 dims = (modelDims == glm::vec3(0.0f)) ? (aabbMax - aabbMin) : modelDims;
        targetSize = Vector3(
            dims.x * (targetSize.x == 0 ? 1.0f : targetSize.x),
            dims.y * (targetSize.y == 0 ? 1.0f : targetSize.y),
            dims.z * (targetSize.z == 0 ? 1.0f : targetSize.z)
        );
    }
    if (aabbComputed) {
        glm::vec3 dims = aabbMax - aabbMin;
        glm::vec3 center = (aabbMin + aabbMax) * 0.5f;
        if (modelDims == glm::vec3(0.0f)) {
            modelDims = dims; // stash original import dimensions (Blender units)
        }
        float sx = dims.x != 0.0f ? targetSize.x / dims.x : 1.0f;
        float sy = dims.y != 0.0f ? targetSize.y / dims.y : 1.0f;
        float sz = dims.z != 0.0f ? targetSize.z / dims.z : 1.0f;
        local = glm::translate(local, -center);
        local = glm::scale(local, glm::vec3(sx, sy, sz));
    } else {
        local = glm::scale(local, glm::vec3(targetSize.x, targetSize.y, targetSize.z));
    }

    // World transform: translate then rotate, then apply local centered S/T
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(angle.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(angle.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(angle.z), glm::vec3(0, 0, 1));
    model = model * local;

    // View/Projection from camera component if present
    glm::mat4 view;
    glm::mat4 projection;
    if (object && object->GetScene()) {
        if (auto* cam = CameraComponent::FindActive(object->GetScene())) {
            view = cam->GetViewMatrix();
            projection = cam->GetProjectionMatrix();
        } else {
            // Default camera
            view = glm::lookAt(
                glm::vec3(0.0f, 0.0f, 5.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            projection = glm::perspective(
                glm::radians(60.0f),
                800.0f / 480.0f,
                0.1f,
                100.0f
            );
        }
    }

    glUseProgram(shaderProgram);

    // Upload matrices
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc  = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc  = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc,  1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc,  1, GL_FALSE, glm::value_ptr(projection));

    // Lighting parameters (example values)
    GLint lightDirLoc     = glGetUniformLocation(shaderProgram, "lightDir");
    GLint lightColorLoc   = glGetUniformLocation(shaderProgram, "lightColor");
    GLint ambientColorLoc = glGetUniformLocation(shaderProgram, "ambientColor");
    // Light coming along +Z towards the object
    glUniform3f(lightDirLoc, 0.0f, 0.0f, -1.0f);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(ambientColorLoc, 0.2f, 0.2f, 0.2f);

    // Render each mesh
    for (auto& mesh : meshes) {
        if (!mesh.textures.empty()) {
            // Bind the first texture
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

// ==================== Load model via Assimp ====================
bool Model3DComponent::loadModel(const std::string& path)
{
    Assimp::Importer importer;
    // Use aiProcess_GenSmoothNormals instead of aiProcess_GenNormals
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate  |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    // Determine model directory (to find textures)
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
    // Cache native dimensions once loaded
    if (aabbComputed) {
        modelDims = aabbMax - aabbMin;
    }
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
    bool hasNormals   = mesh->HasNormals(); // Should be true since we use GenSmoothNormals

    // Layout: (position x,y,z) + (normal x,y,z) + (UV x,y)
    // Total 8 floats per vertex
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // Position
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);

        // Update AABB
        aabbMin.x = std::min(aabbMin.x, mesh->mVertices[i].x);
        aabbMin.y = std::min(aabbMin.y, mesh->mVertices[i].y);
        aabbMin.z = std::min(aabbMin.z, mesh->mVertices[i].z);
        aabbMax.x = std::max(aabbMax.x, mesh->mVertices[i].x);
        aabbMax.y = std::max(aabbMax.y, mesh->mVertices[i].y);
        aabbMax.z = std::max(aabbMax.z, mesh->mVertices[i].z);

        // Normal (fallback to 0,0,1 if missing)
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

    // Indices
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

    // Total 8 floats per vertex: (pos.x,pos.y,pos.z, norm.x,norm.y,norm.z, uv.x,uv.y)
    int stride = 8 * sizeof(float);

    // layout(location=0) -> aPos (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // layout(location=2) -> aNormal (vec3), after first 3 floats (position)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // layout(location=1) -> aTexCoord (vec2), after 6 floats (pos+normal)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    entry.numIndices = static_cast<unsigned int>(indices.size());
    
    // Load diffuse textures (or baseColor if needed)
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // std::cout << "Material index " << mesh->mMaterialIndex
        //           << " has " << material->GetTextureCount(aiTextureType_DIFFUSE)
        //           << " diffuse textures\n";
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        entry.textures.insert(entry.textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    }

    meshes.push_back(entry);

    aabbComputed = true;
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
        std::cerr << "Failed to load texture: " << IMG_GetError() << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Texture parameters
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
