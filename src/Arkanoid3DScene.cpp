#include "SceneDeclarations.h"

void Arkanoid3DScene::Init()
{
    // Window and viewport
    Vector2 windowSize(800, 480);
    SetWindowSize(windowSize.x, windowSize.y);
    glViewport(0, 0, windowSize.x, windowSize.y);

    // Camera object
    cameraObj = CreateObject();
    cameraObj->AddComponent(new CameraComponent());
    auto* cam = cameraObj->GetComponent<CameraComponent>();
    cam->SetPerspective(60.0f, windowSize.x / windowSize.y, 0.1f, 100.0f);
    cameraObj->SetPosition(Vector3(0.0f, 60.0f, 105.0f));
    cameraObj->SetRotation(Vector3(30.0f, 0.0f, 0.0f));

    // Board (paddle) — bottom
    board = CreateObject();
    board->AddComponent(new Model3DComponent("Assets/board.fbx"));
    board->SetPosition(Vector3(0.0f, -1.5f, 0.0f));
    // Expect the FBX to be a flat board; show thickness explicitly small
    board->SetSize(Vector3(1,1,1)/200);
    board->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
    board->SetLayer(100);

    // Cube (test block) — top
    // Object* cube = CreateObject();
    // cube->AddComponent(new Model3DComponent("Assets/cube.fbx"));
    // cube->SetPosition(Vector3(0.0f, 1.6f, 0.0f));
    // cube->SetSize(Vector2(0.6f, 0.6f));
    // cube->SetRotation(Vector3(-90.0f, 0.0f, 0.0f));
    // cube->SetLayer(110);

    //Ball (sphere) — middle
    ball = CreateObject();
    ball->AddComponent(new Model3DComponent("Assets/ball.fbx"));
    ball->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    ball->SetSize(Vector2(1.0f, 1.0f));
    ball->SetRotation(Vector3(-90.0f, 0.0f, 0.0f));
    ball->SetLayer(120);
}

void Arkanoid3DScene::Update()
{
    // Keep static for now to avoid orientation confusion
    //board->SetRotation(board->GetAngle()+Vector3(0,1,0)); 

}


