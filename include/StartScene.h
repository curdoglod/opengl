#pragma once
#include "SceneManager.h"
#include "components.h"

class StartScene : public SceneManager {

    void Awake() override; 
    void Init() override; 

private:
    Object* start_button;
    Image* startBttn_image; 
};
