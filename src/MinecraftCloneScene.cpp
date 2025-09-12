#include "SceneDeclarations.h"

void MinecraftCloneScene::Init()
{
    Vector2 windowSize(800, 480);
    SetWindowSize(windowSize.x, windowSize.y);

    // Camera
    Object* camObj = CreateObject();
    camObj->AddComponent(new CameraComponent());
    auto* cam = camObj->GetComponent<CameraComponent>();
    cam->SetPerspective(60.0f, windowSize.x / windowSize.y, 0.1f, 200.0f);
    camObj->SetPosition(Vector3(0.0f, 80.0f, 120.0f));
    camObj->SetRotation(Vector3(25.0f, 0.0f, 0.0f));

    // Light
    Object* lightObj = CreateObject();
    auto* light = new LightComponent();
    light->SetDirection(glm::vec3(0.3f, -1.0f, 0.2f));
    light->SetColor(glm::vec3(1.0f, 1.0f, 1.0f));
    light->SetAmbient(glm::vec3(0.25f, 0.25f, 0.25f));
    light->SetShadowEnabled(true);
    light->SetShadowMapSize(2048, 2048);
    lightObj->AddComponent(light);

    // Placeholder ground block
    Object* ground = CreateObject();
    ground->AddComponent(new Model3DComponent("Assets/board.fbx"));
    ground->SetPosition(Vector3(0.0f, -1000.0f, 0.0f));
    ground->SetSize(Vector3(2.0f, 0.5f, 2.0f));
    ground->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
}

void MinecraftCloneScene::Update()
{
}
