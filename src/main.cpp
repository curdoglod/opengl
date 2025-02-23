#include <iostream>
#include "engine.h"
#include "SceneDeclarations.h"







class Game : public Engine {

    void Init() {
        SetWindowTitle("Game"); 
        SetWindowSize(400, 700);
        ChangeScene(new StartScene());
     }

};





int main(int argc, char* argv[]) 
{
    Game game; 
    game.Run();

    return 0;
}

