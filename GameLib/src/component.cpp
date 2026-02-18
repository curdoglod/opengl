#include "component.h"
#include "object.h"
#include "Scene.h"

Object* Component::CreateObject() {
    if (!object) return nullptr;
    return object->GetScene()->CreateObject();
}
