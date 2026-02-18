#pragma once
#include "component.h"
#include <glm/glm.hpp>

class CameraComponent : public Component {
public:
    void Init() override {}
    void Update(float dt) override {}

    // Camera parameters
    void SetPerspective(float fovDeg, float aspect, float nearZ, float farZ) {
        fov = fovDeg; aspectRatio = aspect; nearPlane = nearZ; farPlane = farZ;
    }
    void SetActive(bool enabled) { active = enabled; }
    bool IsActive() const { return active; }

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    // Static access to the first active camera in the scene
    static CameraComponent* FindActive(Scene* scene);

private:
    float fov = 60.0f;
    float aspectRatio = 800.0f / 480.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    bool active = true;
};


