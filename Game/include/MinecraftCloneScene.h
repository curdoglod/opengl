#pragma once
#include "Scene.h"
#include "components.h"
#include "HotbarComponent.h"

class MinecraftCloneScene : public Scene
{
public:
    void Init() override;
    void Update() override;
    void onKeyReleased(SDL_Keycode key) override
    {
        if (key == SDLK_q)
        {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            SwitchToScene(new StartScene());
        }
        if (key == SDLK_ESCAPE)
        {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
    }

private:
    Object *camObj;
    Object *lightObj;
    Object *world;
    WorldGridComponent *grid;
    Object *hotbarObj;
    HotbarComponent *hotbar;
};
