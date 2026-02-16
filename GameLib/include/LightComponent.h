#pragma once
#include "component.h"
#include <glm/glm.hpp>
#include <GL/glew.h>

class SceneManager;

class LightComponent : public Component {
public:
    LightComponent();
    ~LightComponent();

    void Init() override;
    void Update(float dt) override;
    void LateUpdate(float dt) override;

    void SetDirection(const glm::vec3& dir) { direction = glm::normalize(dir); }
    void SetColor(const glm::vec3& col) { color = col; }
    void SetAmbient(const glm::vec3& amb) { ambient = amb; }
    void SetShadowEnabled(bool enabled) { enableShadows = enabled; }
    void SetShadowMapSize(int w, int h) { shadowWidth = w; shadowHeight = h; }

    glm::vec3 GetDirection() const { return direction; }
    glm::vec3 GetColor() const { return color; }
    glm::vec3 GetAmbient() const { return ambient; }
    bool IsShadowEnabled() const { return enableShadows; }

    GLuint GetDepthTexture() const { return depthTexture; }

    glm::mat4 GetLightView() const { return lightView; }
    glm::mat4 GetLightProj() const { return lightProj; }
    glm::mat4 GetLightVP() const { return lightProj * lightView; }

    void RenderShadowMap(SceneManager* scene);

    static LightComponent* FindActive(SceneManager* scene);

private:
    void ensureShadowResources();
    void computeLightMatrices(SceneManager* scene);

private:
    glm::vec3 direction;
    glm::vec3 color;
    glm::vec3 ambient;

    bool enableShadows;
    int shadowWidth;
    int shadowHeight;

    GLuint depthFBO;
    GLuint depthTexture;

    glm::mat4 lightView;
    glm::mat4 lightProj;
};
