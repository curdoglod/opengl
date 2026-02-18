#include "LightComponent.h"
#include "Renderer.h"
#include "Scene.h"
#include "object.h"
#include "Model3DComponent.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cmath>

namespace {
GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok=0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok){ char log[512]; glGetShaderInfoLog(s,512,nullptr,log); std::cerr << "Light shader compile: "<<log<<std::endl; }
    return s;
}
GLuint link(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p,vs); glAttachShader(p,fs);
    glLinkProgram(p);
    GLint ok=0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if(!ok){ char log[512]; glGetProgramInfoLog(p,512,nullptr,log); std::cerr << "Light program link: "<<log<<std::endl; }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}
}

static GLuint depthProgram = 0;

LightComponent::LightComponent()
    : direction(glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f)))
    , color(1.0f, 1.0f, 1.0f)
    , ambient(0.2f, 0.2f, 0.2f)
    , enableShadows(true)
    , shadowWidth(1024)
    , shadowHeight(1024)
    , depthFBO(0)
    , depthTexture(0)
    , lightView(1.0f)
    , lightProj(1.0f)
{}

LightComponent::~LightComponent()
{
    if (depthTexture) glDeleteTextures(1, &depthTexture);
    if (depthFBO) glDeleteFramebuffers(1, &depthFBO);
}

void LightComponent::Init()
{
    if (depthProgram == 0) {
        const char* vsSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 model;
uniform mat4 lightVP;
void main(){
    gl_Position = lightVP * model * vec4(aPos,1.0);
}
)";
        const char* fsSrc = R"(
#version 330 core
void main(){ }
)";
        depthProgram = link(compile(GL_VERTEX_SHADER, vsSrc), compile(GL_FRAGMENT_SHADER, fsSrc));
    }
    ensureShadowResources();
}

void LightComponent::ensureShadowResources()
{
    if (!enableShadows) return;
    if (depthFBO == 0) glGenFramebuffers(1, &depthFBO);
    if (depthTexture == 0) {
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0,1.0,1.0,1.0};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow framebuffer not complete" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightComponent::computeLightMatrices(Scene* scene)
{
    // Compute world-space AABB from all scene models
    glm::vec3 wsMin( 1e9f), wsMax(-1e9f);
    bool any = false;
    const auto& objects = scene->GetObjects();
    for (auto* obj : objects) {
        auto* comp = obj->GetComponent<Model3DComponent>();
        if (!comp || !comp->HasAabb()) continue;
        glm::mat4 model = comp->ComputeModelMatrix();
        glm::vec3 mn = comp->GetAabbMin();
        glm::vec3 mx = comp->GetAabbMax();
        glm::vec3 corners[8] = {
            {mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z},
            {mn.x, mn.y, mx.z}, {mx.x, mn.y, mx.z}, {mn.x, mx.y, mx.z}, {mx.x, mx.y, mx.z}
        };
        for (int i = 0; i < 8; ++i) {
            glm::vec4 w = model * glm::vec4(corners[i], 1.0f);
            wsMin = glm::min(wsMin, glm::vec3(w));
            wsMax = glm::max(wsMax, glm::vec3(w));
            any = true;
        }
    }
    if (!any) {
        glm::vec3 lightPos = glm::vec3(0.0f) - direction * 50.0f;
        glm::vec3 up = glm::vec3(0, 1, 0);
        lightView = glm::lookAt(lightPos, glm::vec3(0.0f), up);
        float orthoRange = 50.0f;
        lightProj = glm::ortho(-orthoRange, orthoRange, -orthoRange, orthoRange, 0.1f, 150.0f);
        return;
    }

    // Use a STABLE center and radius that don't change frame-to-frame
    // Round the scene bounds to fixed increments so the frustum doesn't jitter
    glm::vec3 center = (wsMin + wsMax) * 0.5f;
    glm::vec3 extents = (wsMax - wsMin) * 0.5f;
    float radius = glm::length(extents);
    if (radius < 1.0f) radius = 1.0f;

    // Quantize radius to prevent jittering when blocks are added/removed
    radius = std::ceil(radius);

    // Build the light view matrix from a stable direction
    glm::vec3 up = (std::abs(direction.y) > 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    glm::vec3 lightPos = center - direction * (radius + 5.0f);
    lightView = glm::lookAt(lightPos, center, up);

    // Compute light-space bounds of all scene geometry
    glm::vec3 lsMin( 1e9f), lsMax(-1e9f);
    for (auto* obj : objects) {
        auto* comp = obj->GetComponent<Model3DComponent>();
        if (!comp || !comp->HasAabb()) continue;
        glm::mat4 model = comp->ComputeModelMatrix();
        glm::vec3 mn = comp->GetAabbMin();
        glm::vec3 mx = comp->GetAabbMax();
        glm::vec3 corners[8] = {
            {mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z},
            {mn.x, mn.y, mx.z}, {mx.x, mn.y, mx.z}, {mn.x, mx.y, mx.z}, {mx.x, mx.y, mx.z}
        };
        for (int i = 0; i < 8; ++i) {
            glm::vec4 w = model * glm::vec4(corners[i], 1.0f);
            glm::vec4 l = lightView * w;
            lsMin = glm::min(lsMin, glm::vec3(l));
            lsMax = glm::max(lsMax, glm::vec3(l));
        }
    }
    float margin = 1.0f;
    float lsLeft   = lsMin.x - margin;
    float lsRight  = lsMax.x + margin;
    float lsBottom = lsMin.y - margin;
    float lsTop    = lsMax.y + margin;
    float lsNear   = 0.1f;
    float lsFar    = (lsMax.z - lsMin.z) + 10.0f;

    // ---- Snap ortho frustum to shadow-texel boundaries (eliminates shimmer) ----
    // The world-space size of a single shadow texel:
    float frustumWidth  = lsRight - lsLeft;
    float frustumHeight = lsTop - lsBottom;
    float texelSizeX = frustumWidth  / (float)shadowWidth;
    float texelSizeY = frustumHeight / (float)shadowHeight;

    // Snap the ortho bounds so that moving the camera never shifts shadows
    // by a sub-texel amount — this eliminates the "swimming" artifact.
    lsLeft   = std::floor(lsLeft   / texelSizeX) * texelSizeX;
    lsRight  = std::ceil (lsRight  / texelSizeX) * texelSizeX;
    lsBottom = std::floor(lsBottom / texelSizeY) * texelSizeY;
    lsTop    = std::ceil (lsTop    / texelSizeY) * texelSizeY;

    lightProj = glm::ortho(lsLeft, lsRight, lsBottom, lsTop, lsNear, lsFar);
}

void LightComponent::RenderShadowMap(Scene* scene)
{
    if (!enableShadows || !scene) return;
    ensureShadowResources();
    computeLightMatrices(scene);

    glViewport(0,0,shadowWidth,shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(depthProgram);
    GLint lightVPLoc = glGetUniformLocation(depthProgram, "lightVP");
    glm::mat4 lightVP = GetLightVP();
    glUniformMatrix4fv(lightVPLoc, 1, GL_FALSE, glm::value_ptr(lightVP));

    const auto& objects = scene->GetObjects();
    for (auto* obj : objects) {
        auto* modelComp = obj->GetComponent<Model3DComponent>();
        if (!modelComp) continue;
        glm::mat4 model = modelComp->ComputeModelMatrix();
        modelComp->RenderDepthPass(model, depthProgram);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glViewport(0, 0, Renderer::Get().GetWindowWidth(), Renderer::Get().GetWindowHeight());
}

LightComponent* LightComponent::FindActive(Scene* scene)
{
    if (!scene) return nullptr;
    const auto& objects = scene->GetObjects();
    for (auto* obj : objects) {
        auto* light = obj->GetComponent<LightComponent>();
        if (light) return light; // first light
    }
    return nullptr;
}
