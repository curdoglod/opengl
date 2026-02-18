#pragma once

#include "Scene.h"
#include "components.h"

class Arkanoid3DScene : public Scene {
public:
    void Init() override;
    void Update() override;

    void onKeyReleased(SDL_Keycode key) override {
        if (key == SDLK_ESCAPE) {
            SwitchToScene(new StartScene());
        }
    }
    void onKeyPressed(SDL_Keycode key) override; 
private:
    Object* board;
    Object* ball;
    Object* cameraObj;
    Object* lightObj;
};


