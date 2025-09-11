#include "engine.h"
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <algorithm>
#include "SceneManager.h"
#include <chrono>
#include <GL/glew.h>

struct Engine::Impl
{
    Impl() {}
    void preInit()
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        m_window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        // m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
        SDL_GLContext glContext = SDL_GL_CreateContext(m_window);
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
        {
            std::cerr << "GLEW initialization error!" << std::endl;
            // error handling
        }
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
          glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void Tick(float deltaTime)
    {

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                SDL_DestroyWindow(m_window);
                m_window = nullptr;
            }

            if (currentScene != nullptr)
                currentScene->UpdateEvents(event);
        }

        // SDL_SetRenderDrawColor(m_renderer, 0, 100, 255, 255);
        // SDL_RenderClear(m_renderer);
        
        glClearColor(0, 100 / 255.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (currentScene != nullptr)
            currentScene->UpdateScene(deltaTime);

        SDL_GL_SwapWindow(m_window);

        ///  SDL_RenderPresent(m_renderer);
    }

    int FPS = 60;
    SDL_Window *m_window = nullptr;
    SDL_Renderer *m_renderer = nullptr;
    std::string nameWindow;

    SceneManager *currentScene = nullptr;
    SceneManager *prevScene = nullptr;

    static ArchiveUnpacker *DefaultArchive;
    static ArchiveUnpacker *ResourcesArchive;
};

ArchiveUnpacker *Engine::Impl::DefaultArchive = nullptr;
ArchiveUnpacker *Engine::Impl::ResourcesArchive = nullptr;

Engine::Engine() : impl(new Impl())
{
    // Initialize SDL
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    impl->DefaultArchive = new ArchiveUnpacker("DefaultAssets");
    impl->ResourcesArchive = new ArchiveUnpacker("Assets");
    impl->DefaultArchive->Unpack();
    impl->ResourcesArchive->Unpack();
    impl->FPS = 60;
}

void Engine::Run()
{
    impl->preInit();

    Init();

    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    int frameDelay = 1000 / impl->FPS;

    while (impl->m_window != nullptr)
    {
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> frameDuration = frameStart - lastFrameTime;
        lastFrameTime = frameStart;

        impl->Tick(frameDuration.count() / 1000.0f);

        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> elapsedTime = frameEnd - frameStart;
        int elapsedMS = static_cast<int>(elapsedTime.count());

        if (frameDelay > elapsedMS)
        {
            SDL_Delay(frameDelay - elapsedMS);
        }
    }
}

// void Engine::Run() {
//     preInit();
//     init();
//
//
//      int frameDelay = 1000 / FPS;
//
//     Uint32 frameStart;
//     int frameTime=0.0001;
//
//     while (m_window != nullptr) {
//         frameStart = SDL_GetTicks();
//         int frameDelay = 1000 / FPS;
//
//         Tick(frameTime / 1000.0f);
//
//
//         frameTime = SDL_GetTicks() - frameStart;
//
//         if (frameDelay > frameTime) {
//             SDL_Delay(frameDelay - frameTime);
//         }
//     }
// }

void Engine::ChangeScene(SceneManager *newScene)
{

    if (impl->currentScene)
    {
        if (impl->prevScene)
            delete impl->prevScene;
        impl->prevScene = impl->currentScene;
    }

    impl->currentScene = newScene;
    impl->currentScene->PreInit(this, impl->m_renderer, impl->m_window);
    impl->currentScene->Awake();
    impl->currentScene->Init();
}

ArchiveUnpacker *Engine::GetDefaultArchive()
{
    return Engine::Impl::DefaultArchive;
}

ArchiveUnpacker *Engine::GetResourcesArchive()
{
    return Engine::Impl::ResourcesArchive;
}

void Engine::SetFPS(const int &fps) { impl->FPS = fps; };

void Engine::SetWindowSize(const int &w, const int &h)
{
    SDL_SetWindowSize(impl->m_window, w, h);
}

void Engine::SetWindowTitle(const std::string &newTitle)
{
    SDL_SetWindowTitle(impl->m_window, newTitle.c_str());
}

void Engine::Quit()
{
    SDL_DestroyRenderer(impl->m_renderer);
    SDL_DestroyWindow(impl->m_window);
    IMG_Quit();
    SDL_Quit();
}

Engine::~Engine()
{
    SDL_DestroyRenderer(impl->m_renderer);
    SDL_DestroyWindow(impl->m_window);
    IMG_Quit();
    SDL_Quit();
    delete impl->prevScene;
    delete impl->currentScene;
    delete impl->DefaultArchive;
    delete impl->ResourcesArchive;
    delete impl;
}
