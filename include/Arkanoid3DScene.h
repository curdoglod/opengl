#pragma once

#include "SceneManager.h"
#include "components.h"

class Arkanoid3DScene : public SceneManager {
public:
    void Init() override;
    void Update() override;

    void onKeyReleased(SDL_Keycode key) override {
        if (key == SDLK_ESCAPE) {
            SwitchToScene(new StartScene());
        }
    }
    void onKeyPressed(SDL_Keycode key) override {
        if (key == SDLK_w) {
            cameraObj->SetPosition(cameraObj->GetPosition3D()+Vector3(0,1,0));
            //cameraObj->SetRotation(cameraObj->GetAngle()+Vector3(-1,0,0));
        }
        if (key == SDLK_s) {
            cameraObj->SetPosition(cameraObj->GetPosition3D()+Vector3(0,-1,0));
            //cameraObj->SetRotation(cameraObj->GetAngle()+Vector3(1,0,0));
        }
        if (key == SDLK_a) {
            cameraObj->SetPosition(cameraObj->GetPosition3D()+Vector3(-1,0,0));
            //cameraObj->SetRotation(cameraObj->GetAngle()+Vector3(0,-1,0));
        }
        if (key == SDLK_d) {
            cameraObj->SetPosition(cameraObj->GetPosition3D()+Vector3(1,0,0));
            //cameraObj->SetRotation(cameraObj->GetAngle()+Vector3(0,1,0));
        }
    }
private:
    Object* board;
    Object* ball;
    Object* cameraObj;
};


