#include "Model3DComponent.h"
#include "object.h"
#include "ResourceManager.h"
#include "LightComponent.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL.h>
#include <iostream>

// ==================== Shaders (Lambert lighting + basic shadows) ====================
static const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPos;       // Vertex position
layout(location = 1) in vec2 aTexCoord;  // UV
layout(location = 2) in vec3 aNormal;    // Normal

out vec2 TexCoord;
out vec3 Normal;     
out vec3 FragPos;
out vec4 LightSpacePos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightVP;

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

    LightSpacePos = lightVP * worldPos;
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 LightSpacePos;

// Textures
uniform sampler2D ourTexture;
uniform sampler2D shadowMap;

// Light parameters
uniform vec3 lightDir;      // light direction (world)
uniform vec3 lightColor;    // light color
uniform vec3 ambientColor;  // ambient color
uniform int useShadows;     // 0/1
uniform vec4 highlightTint; // rgb=tint color, a=mix factor (0=no tint)

float ShadowCalculation(vec4 lightSpacePos, vec3 normal, vec3 lightDirection)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;

    // Slope-scaled bias: small values to keep shadow close to geometry
    float cosTheta = max(dot(normalize(normal), -normalize(lightDirection)), 0.0);
    float bias = mix(0.002, 0.0004, cosTheta);

    // 5x5 PCF for softer shadow edges
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int samples = 0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
            samples++;
        }
    }
    shadow /= float(samples);

    // Fade shadow near shadow-map borders to avoid hard cutoff
    float fadeRange = 0.05;
    float fadeFactor = 1.0;
    fadeFactor *= smoothstep(0.0, fadeRange, projCoords.x);
    fadeFactor *= smoothstep(0.0, fadeRange, 1.0 - projCoords.x);
    fadeFactor *= smoothstep(0.0, fadeRange, projCoords.y);
    fadeFactor *= smoothstep(0.0, fadeRange, 1.0 - projCoords.y);
    shadow *= fadeFactor;

    return shadow;
}

void main()
{
    vec3 texColor = texture(ourTexture, TexCoord).rgb;
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 ambient = ambientColor;

    float shadow = 0.0;
    if (useShadows == 1) {
        shadow = ShadowCalculation(LightSpacePos, norm, lightDir);
    }

    vec3 result = texColor * (ambient + (1.0 - shadow) * diffuse);
    result = mix(result, highlightTint.rgb, highlightTint.a);
    FragColor   = vec4(result, 1.0);
}
)";

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

glm::mat4 Model3DComponent::ComputeModelMatrix() const
{
    Vector3 p = object->GetPosition3D();
    glm::vec3 position(p.x, p.y, p.z);
    Vector3 angle = object->GetAngle();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 local = glm::mat4(1.0f);

    Vector3 targetSize = object->GetSize3D();
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
            // const_cast is safe-ish here to initialize cache; but avoid mutating.
        }
        float sx = dims.x != 0.0f ? targetSize.x / dims.x : 1.0f;
        float sy = dims.y != 0.0f ? targetSize.y / dims.y : 1.0f;
        float sz = dims.z != 0.0f ? targetSize.z / dims.z : 1.0f;
        // First translate to center the model at origin, then scale
        local = glm::scale(local, glm::vec3(sx, sy, sz));
        local = glm::translate(local, -center);
    } else {
        local = glm::scale(local, glm::vec3(targetSize.x, targetSize.y, targetSize.z));
    }

    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(angle.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(angle.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(angle.z), glm::vec3(0, 0, 1));
    model = model * local;
    return model;
}

void Model3DComponent::RenderDepthPass(const glm::mat4& model, GLuint depthProgram) const
{
    GLint modelLoc = glGetUniformLocation(depthProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    for (const auto& mesh : meshes) {
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

bool Model3DComponent::SetAlbedoTextureFromFile(const std::string& fullPath)
{
    GLuint id = ResourceManager::Get().LoadTexture(fullPath);
    if (id == 0) return false;
    overrideAlbedoTexture = id;
    return true;
}

// Returns a 1×1 white texture used as a fallback when no shadow map is available.
// This prevents the GPU driver from warning about an unbound sampler on unit 1.
// We use GL_RED (not GL_DEPTH_COMPONENT) because macOS's OpenGL driver may reject
// incomplete depth textures bound to a regular sampler2D.
static GLuint getDummyShadowMap() {
    static GLuint dummy = 0;
    if (dummy == 0) {
        glGenTextures(1, &dummy);
        glBindTexture(GL_TEXTURE_2D, dummy);
        unsigned char white = 255;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return dummy;
}

// ==================== Render: called by RenderSystem ==========================
void Model3DComponent::Render(const glm::mat4& view, const glm::mat4& projection, LightComponent* light)
{
    glm::mat4 model = ComputeModelMatrix();

    GLuint prog = ResourceManager::Get().GetOrCreateShader("model3d", vertexShaderSource, fragmentShaderSource);
    glUseProgram(prog);

    // Upload matrices
    GLint modelLoc = glGetUniformLocation(prog, "model");
    GLint viewLoc  = glGetUniformLocation(prog, "view");
    GLint projLoc  = glGetUniformLocation(prog, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc,  1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc,  1, GL_FALSE, glm::value_ptr(projection));

    // Lighting parameters
    glm::vec3 lightDir(0.0f, 0.0f, -1.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 ambientColor(0.2f, 0.2f, 0.2f);
    glm::mat4 lightVP(1.0f);
    int useShadows = 0;

    if (light) {
        lightDir = light->GetDirection();
        lightColor = light->GetColor();
        ambientColor = light->GetAmbient();
        lightVP = light->GetLightVP();
        useShadows = light->IsShadowEnabled() && (light->GetDepthTexture() != 0) ? 1 : 0;
    }

    // Always bind a valid texture to unit 1 so the sampler is never unbound.
    // Use the real shadow map when available, otherwise a 1×1 dummy depth texture.
    GLuint shadowTex = (light && light->GetDepthTexture() != 0)
                       ? light->GetDepthTexture()
                       : getDummyShadowMap();
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glUniform1i(glGetUniformLocation(prog, "shadowMap"), 1);

    GLint lightDirLoc     = glGetUniformLocation(prog, "lightDir");
    GLint lightColorLoc   = glGetUniformLocation(prog, "lightColor");
    GLint ambientColorLoc = glGetUniformLocation(prog, "ambientColor");
    GLint lightVPLoc      = glGetUniformLocation(prog, "lightVP");
    GLint useShadowsLoc   = glGetUniformLocation(prog, "useShadows");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(ambientColorLoc, 1, glm::value_ptr(ambientColor));
    glUniformMatrix4fv(lightVPLoc, 1, GL_FALSE, glm::value_ptr(lightVP));
    glUniform1i(useShadowsLoc, useShadows);
    glUniform4fv(glGetUniformLocation(prog, "highlightTint"), 1, glm::value_ptr(highlightTint));

    // Bind albedo texture: prefer override if present, otherwise first mesh texture
    for (auto& mesh : meshes) {
        GLuint albedoTex = overrideAlbedoTexture ? overrideAlbedoTexture : (mesh.textures.empty() ? 0 : mesh.textures[0].id);
        if (albedoTex != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, albedoTex);
            glUniform1i(glGetUniformLocation(prog, "ourTexture"), 0);
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
        Texture texture;
        texture.id   = TextureFromFile(str.C_Str(), directory);
        texture.type = typeName;
        texture.path = str.C_Str();
        textures.push_back(texture);
    }
    return textures;
}

GLuint Model3DComponent::TextureFromFile(const char* path, const std::string& directory)
{
    std::string filename = directory + "/" + std::string(path);
    return ResourceManager::Get().LoadTexture(filename);
}
