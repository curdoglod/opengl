#ifndef OBJECT_H
#define OBJECT_H

#include <SDL.h>
#include <iostream>
#include <vector>
#include "sprite.h"
#include "engine.h"
#include "Utils.h"


class Component; 
class Image; 
class SceneManager; 

class Object {
public:
    bool Crossing(Object* obj, const float& x_range, const float& y_range);
    bool Crossing(Object* obj);
    Vector2 GetPosition() const;
    float GetAngle() const {return angle; }; 
    void SetPosition(const Vector2& vec2);
    void SetRotation(float angle);

    void SetPositionOnPlatform(const Vector2 & vec2);
    void MoveY(const float& pos_y);
    void MoveX(const float& pos_x);

    Vector2 GetSize();
    void InitSize(Image* img);
    void InitSize();
    void SetLayer(int layer);
    int GetLayer() const;
    void AddComponent(Component* component);
    Component* GetComponent(const std::type_info& ti) const;

    template<typename T>
    void RemoveComponent();
  
    void update(float deltaTime);
    SceneManager* GetScene() const;
    SDL_Renderer* GetRenderer() const;
    void UpdateEvents(SDL_Event& event);
    void SetActive(bool status);
    template <typename T>
    T* GetComponent() const {
        Component* comp = this->GetComponent(typeid(T));
        return dynamic_cast<T*>(comp);
    }
    Object* CloneObject() const;

    friend class Engine; 
    friend class SceneManager;
private:
    virtual ~Object();
    Object(SceneManager* _game, SDL_Renderer* renderer_);
    SDL_Renderer* renderer;
    SceneManager* currentScene;
    std::vector<Component*> components;
    Vector2 position;
    Vector2 size;
    float angle; 
    int layer;
    bool active; 
    float deltatime;
   
  


};

#endif // OBJECT_H
