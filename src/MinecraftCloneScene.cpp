#include "SceneDeclarations.h"

void MinecraftCloneScene::Init()
{
    Vector2 windowSize(800, 480);
    SetWindowSize(windowSize.x, windowSize.y);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    
    camObj = CreateObject();
    camObj->AddComponent(new CameraComponent());
    auto* cam = camObj->GetComponent<CameraComponent>();
    cam->SetPerspective(60.0f, windowSize.x / windowSize.y, 1.0f, 1000.0f);
    camObj->SetRotation(Vector3(50.0f, 0.0f, 0.0f));
    
    lightObj = CreateObject();
    auto* light = new LightComponent();
    light->SetDirection(glm::vec3(0.3f, -1.0f, 0.2f));
    light->SetColor(glm::vec3(1.0f, 1.0f, 1.0f));
    light->SetAmbient(glm::vec3(0.25f, 0.25f, 0.25f));
    light->SetShadowEnabled(true);
    light->SetShadowMapSize(1024, 1024);
    lightObj->AddComponent(light);

    world = CreateObject();
    grid = new WorldGridComponent();
    grid->SetSize(24, 24);
    grid->SetBlockSize(20.0f);
    world->AddComponent(grid);
    grid->GenerateHillyTerrain(1, 1, BlockType::Dirt, BlockType::Stone);

    Object* player = CreateObject();
    player->SetPosition(Vector3(0.0f, 40.0f, 120.0f));
    player->SetSize(Vector3(1,10,1)/100);
    auto* pc = new PlayerController();
    pc->SetMoveSpeed(160.0f);
    pc->SetCamera(camObj);
    Vector3 camOffset(0.0f, 40.0f, 10.0f);
    pc->SetCameraOffset(camOffset);
    player->AddComponent(pc);
    player->SetLayer(-1000);
    camObj->SetPosition(player->GetPosition3D() + camOffset);
    camObj->SetRotation(Vector3(30.0f, 0.0f, 0.0f));

    Object* crosshair = CreateObject();
    crosshair->SetPosition(Vector2(windowSize.x * 0.5f, windowSize.y * 0.5f));
    crosshair->SetSize(Vector2(16, 16));
    crosshair->SetLayer(1000);
    crosshair->AddComponent(new Image(Engine::GetResourcesArchive()->GetFile("ball.png")));
}

void MinecraftCloneScene::Update()
{
}