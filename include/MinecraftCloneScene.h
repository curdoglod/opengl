#pragma once
#include "SceneManager.h"
#include "components.h"

class MinecraftCloneScene : public SceneManager {
public:
    void Init() override;
    void Update() override;
    void onKeyReleased(SDL_Keycode key) override {
        if (key == SDLK_ESCAPE) {
            SwitchToScene(new StartScene());
        }
    }
private:
    Object* camObj;
    Object* lightObj;
    Object* world;
    WorldGridComponent* grid;
};
