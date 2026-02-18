#include "SceneDeclarations.h"

void StartScene::Awake()
{
}

void StartScene::Init()
{
    my3DObject = CreateObject();
    // Add 3D model component (FBX/OBJ/etc.)
    my3DObject->AddComponent(new Model3DComponent("Assets/model.fbx"));

    // Set position and size (scale)
    my3DObject->SetPosition(Vector3(50.0f / 35.0f, 10.0f / 35.0f, 100.0f / 35.0f));
    //my3DObject->SetSize(Vector2(1, 1)); // scale 1:1
    my3DObject->SetRotation(Vector3(-90,0,0)); 
    my3DObject->SetLayer(200); 
    Vector2 windowSize(1280, 720);
    SetWindowSize(windowSize.x, windowSize.y);
    glViewport(0, 0, windowSize.x, windowSize.y);
    UIdraw();
}
void StartScene::UIdraw()
{
    Vector2 windowSize(1280, 720);

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

    Object *startArkanoid3D_button = start_button->CloneObject();
    startArkanoid3D_button->MoveY(startBttn_image->GetSize().y * 2.4f);
    startArkanoid3D_button->GetComponent<ButtonComponent>()->SetOnClick([this]()
                                                                        { SwitchToScene(new Arkanoid3DScene()); });
    startArkanoid3D_button->GetComponent<TextComponent>()->setText("Arkanoid 3D");

    Object *startMinecraftClone_button = start_button->CloneObject();
    startMinecraftClone_button->MoveY(startBttn_image->GetSize().y * 3.6f);
    startMinecraftClone_button->GetComponent<ButtonComponent>()->SetOnClick([this]()
                                                                            { SwitchToScene(new MinecraftCloneScene()); });
    startMinecraftClone_button->GetComponent<TextComponent>()->setText("Minecraft Clone");
}
void StartScene::Update()
{
    my3DObject->SetRotation(my3DObject->GetAngle()+Vector3(1,1,1)); 
}
