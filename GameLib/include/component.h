#ifndef COMPONENT_H
#define COMPONENT_H
#include "Utils.h"
#include <SDL.h>

class Object;

class Component {
public:
    
    virtual ~Component() {}

    virtual void Init() {}

    virtual void Update() {
    }
    virtual void Update(float deltaTime) {
    }

    // Called after ALL objects have run Update().
    // Use for logic that depends on the final state of other objects
    // this frame (e.g. camera following). Rendering is handled by RenderSystem.
    virtual void LateUpdate(float deltaTime) {
    }
 
    virtual void OnMouseButtonDown(Vector2 mouse_position) {

    }

    virtual void OnMouseButtonUp(Vector2 mouse_position) {

    }
    virtual void OnMouseButtonMotion(Vector2 mouse_position) {

    }
    virtual void onKeyPressed(SDL_Keycode key) {}
    virtual void onKeyReleased(SDL_Keycode key) {}

    // --- Collision callbacks (called by the physics system) ----------------
    // Solid-vs-solid collision (non-trigger).
    virtual void OnCollisionEnter(Object* other) {}
    // Trigger overlap.
    virtual void OnTriggerEnter(Object* other) {}

    virtual Component* Clone() const { return nullptr; }
   
protected:
    Object* object = nullptr;
    Object* CreateObject();
private:
    friend class Object; 
    void setOwner(Object* owner) {
        object = owner;
    }
}; 




#endif // COMPONENT_H