#ifndef SCENE_H
#define SCENE_H


#include "object.h"
#include <vector>
#include "engine.h"
#include <algorithm>
#include <SDL_image.h>
#include "ArchiveUnpacker.h"
#include "Renderer.h"

class SceneManager {
public:
   
    virtual ~SceneManager() {
        for (auto* object : objects) {
            delete object; 
        }
        objects.clear();
    }

    void PreInit(Engine* engine, SDL_Window* window) {
        m_window = window; 
        m_engine = engine;
        SetWindowSize((int)GetWindowSize().x, (int)GetWindowSize().y);
    }

    virtual void Awake() {}

    virtual void Init() {}

    virtual void Update() {}
    
    void UpdateEvents(SDL_Event event) {
      
        // Iterate over a snapshot so that handlers can safely
        // create / delete objects without invalidating the loop.
        auto snapshot = objects;
        for (auto& object : snapshot) {
            if (object->active) {
                object->UpdateEvents(event);
            }

        }

        switch (event.type) {
        case SDL_MOUSEMOTION:
            onMouseMove(event.motion.x, event.motion.y);
            break;

        case SDL_MOUSEBUTTONDOWN:
            onMouseButtonDown(event.button.button, event.button.x, event.button.y);
            break;

        case SDL_MOUSEBUTTONUP:
            onMouseButtonUp(event.button.button, event.button.x, event.button.y);
            break;

        case SDL_KEYDOWN:
            onKeyPressed(event.key.keysym.sym);
            break;

        case SDL_KEYUP:
            onKeyReleased(event.key.keysym.sym);
            break;
        }

   }

   void UpdateScene(float deltaTime) {
       Update();
        // Pass 1: Logic update (movement, physics, input handling, etc.)
        for (auto& object : objects) {
            if (object->active) {
                object->update(deltaTime);
            }
        }
        // Pass 2: Late update / rendering (all objects see the final camera
        // position, light matrices, etc. for this frame).
        for (auto& object : objects) {
            if (object->active) {
                object->lateUpdate(deltaTime);
            }
        }
    }

   Object* CreateObject() {
       Object* object = new Object(this);
        objects.push_back(object);
        updateLayer();

        return object;
    }
   
   void DeleteObject(Object* object) {

       for (auto it = objects.begin(); it != objects.end(); ++it) {
           if (*it == object) {
               delete* it;
               objects.erase(it);
               updateLayer(); 
               break; 
           }
       }
   }


   Sprite* createSprite(const std::vector<unsigned char>& imageData) {
    //return new Sprite(imageData, m_renderer);
    return new Sprite(imageData);
   }



   void updateLayer() {
       std::sort(objects.begin(), objects.end(), [](const auto& a, const auto& b) {
           return a->GetLayer() < b->GetLayer();
           });

   }


   Vector2 GetWindowSize() {
       int w, h;
       SDL_GetWindowSize(m_window, &w, &h);
       return Vector2((float)w,(float) h);
   }
  
   void SwitchToScene(SceneManager* newScene) {
       m_engine->ChangeScene(newScene);
   }

   void SetWindowSize(const int& w, const int& h) {
    SDL_SetWindowSize(m_window, w, h);
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    Renderer::Get().SetWindowSize(w, h);
}

   virtual void onMouseMove(int x, int y) {}
   virtual void onMouseButtonDown(Uint8 button, int x, int y) {}
   virtual void onMouseButtonUp(Uint8 button, int x, int y) {}
   virtual void onKeyPressed(SDL_Keycode key) {}
   virtual void onKeyReleased(SDL_Keycode key) {}


   // Expose read-only view of objects for systems like CameraComponent
   const std::vector<Object*>& GetObjects() const { return objects; }

private: 
    std::vector<Object*> objects;
    SDL_Window* m_window;
    Engine* m_engine;
};

#endif // SCENE_H


