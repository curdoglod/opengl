#pragma once



class GameScene :public SceneManager
{
public:
	void Init() override;

	void Update() override;

	void onKeyReleased(SDL_Keycode key) override
	{
		if(key == SDLK_ESCAPE)
		{
			SwitchToScene(new StartScene());
		}
	}

private:
	void GenBackground();
	Vector2 RandomApple();
	Object* apple;
	int block_size; 
	Object* snake; 
	Vector2 block_count;
	Object* scoreObj; 
	int appleCount; 
};

