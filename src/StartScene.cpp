#include "SceneDeclarations.h"

void StartScene::Awake()
{
}

void StartScene::Init()
{
    my3DObject = CreateObject();
    my3DObject->AddComponent(new Model3DComponent("Assets/model.fbx"));

    my3DObject->SetPosition(Vector3(50, 10, 100));
    my3DObject->SetSize(Vector2(1, 1)); // масштаб 1:1
    my3DObject->SetRotation(Vector3(-90,0,0)); 
    my3DObject->SetLayer(200); 
    Vector2 windowSize(800, 480);
    SetWindowSize(windowSize.x, windowSize.y);
    glViewport(0, 0, windowSize.x, windowSize.y);
    UIdraw();
}
void StartScene::UIdraw()
{
    Vector2 windowSize(800, 480);

    Object *background = CreateObject();
    background->AddComponent(new Image(Engine::GetResourcesArchive()->GetFile("block_sgreen.png")));
    background->GetComponent<Image>()->SetSize(windowSize);
    start_button = CreateObject();
    start_button->AddComponent(new ButtonComponent([this]()
                                                   { SwitchToScene(new GameScene()); }));
    startBttn_image = start_button->GetComponent<Image>();
    startBttn_image->SetNewSprite(Engine::GetResourcesArchive()->GetFile("block_tgreen.png"));
    startBttn_image->SetSize(Vector2(150, 50));
    start_button->SetPosition(GetWindowSize() / 2 - startBttn_image->GetSize() / 2);
    start_button->AddComponent(new TextComponent(20, "Snake Game", Color(255, 255, 255), TextAlignment::CENTER));
    
    Object *startPaddleGame_button = start_button->CloneObject();
    startPaddleGame_button->MoveY(startBttn_image->GetSize().y * 1.2f);
    startPaddleGame_button->GetComponent<ButtonComponent>()->SetOnClick([this]()
                                                                        { SwitchToScene(new MainGameScene()); });
    startPaddleGame_button->GetComponent<TextComponent>()->setText("Arkanoid Game");
    
    Object *collisionTest_button = start_button->CloneObject();
    collisionTest_button->MoveY(startBttn_image->GetSize().y * 2.4f);
    collisionTest_button->GetComponent<ButtonComponent>()->SetOnClick([this]()
                                                                     { SwitchToScene(new CollisionTestScene()); });
    collisionTest_button->GetComponent<TextComponent>()->setText("Collision Test");
}
void StartScene::Update()
{
    my3DObject->SetRotation(my3DObject->GetAngle()+Vector3(1,1,1)); 
}
