#include "SceneDeclarations.h"

void StartScene::Awake()
{
}

void StartScene::Init()
{

    Vector2 windowSize(800, 480);
    SetWindowSize(windowSize.x, windowSize.y);
    glViewport(0, 0, windowSize.x, windowSize.y);

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
}
