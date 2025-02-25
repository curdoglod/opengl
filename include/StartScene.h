#pragma once
#include "SceneManager.h"
#include "components.h"

class StartScene : public SceneManager {

    void Awake() override; 
    void Init() override; 
    void Update() override; 
private:
    Object* start_button;
    Image* startBttn_image; 
    Object* my3DObject;
    void UIdraw();
};
