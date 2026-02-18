#include "SceneDeclarations.h"
#include "paddle.h"

void MainGameScene::Init() {
	
	score = 0;

	std::vector<unsigned char> ballImgData = Engine::GetResourcesArchive()->GetFile("ball.png");
	std::vector<unsigned char> blockImgData = Engine::GetResourcesArchive()->GetFile("block.png");

	scoreText = new TextComponent(20, "Score:" + std::to_string(score),  TextAlignment::LEFT);
	scoreObj = CreateObject(); 
	scoreObj->AddComponent(scoreText);
	scoreObj->SetPosition(Vector2(0, 650)); 

	

	paddle = CreateObject();
	paddle->AddComponent(new PaddleComponent());
	
	ball = CreateObject();
	ball->AddComponent(new Image(ballImgData));
	ball->SetPosition(Vector2(200, 200));
	dir_ball = new Vector2(-4, 3);


	blocks.push_back(CreateObject());
	blocks[0]->AddComponent(new Image(blockImgData));




	Vector2* size_block = new Vector2(blocks[0]->GetSize());
	for (int i = 1; i < 50; i++) {
		blocks.push_back(CreateObject());
		blocks[i]->AddComponent(new Image(blockImgData)); 
		blocks[i]->SetPosition(Vector2(
			blocks[0]->GetPosition().x + i % int(GetWindowSize().x / size_block->x + 1) * size_block->x,
			blocks[0]->GetPosition().y + i / int(GetWindowSize().x / size_block->x + 1) * size_block->y));
	}

}

void MainGameScene::Update() {

	if (ball->Crossing(paddle))
		dir_ball->y = -3;
	ball->MoveX(dir_ball->x);

	for (int i = 0; i < blocks.size(); i++) {
		if (ball->Crossing(blocks[i])) {
			score++; scoreText->setText("Score:" + std::to_string(score));

			DeleteObject(blocks[i]);
			blocks.erase(blocks.begin() + i);
			dir_ball->x *= -1; dir_ball->y += -1;
		}
	}


	if (ball->GetPosition().x <= 0) dir_ball->x *= -1;
	else if (ball->GetPosition().x >= GetWindowSize().x - ball->GetSize().x) dir_ball->x *= -1;
	else if (ball->GetPosition().y <= 0) dir_ball->y = 3;


	ball->MoveY(dir_ball->y);

	if (ball->GetPosition().y >= GetWindowSize().y)  SwitchToScene(new MainGameScene());
	if (blocks.size() == 0) SwitchToScene(new StartScene());

}
