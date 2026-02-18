#include "Scene.h"
#include "BoxCollider3D.h"
#include "object.h"

void Scene::dispatchCollisions()
{
    // Gather all active objects that carry a BoxCollider3D
    std::vector<std::pair<Object*, BoxCollider3D*>> colliders;
    for (auto* obj : objects) {
        if (!obj->active) continue;
        auto* col = obj->GetComponent<BoxCollider3D>();
        if (col) colliders.push_back({obj, col});
    }

    // O(n^2) broad-phase – acceptable for small scenes
    for (size_t i = 0; i < colliders.size(); ++i) {
        for (size_t j = i + 1; j < colliders.size(); ++j) {
            auto& [objA, colA] = colliders[i];
            auto& [objB, colB] = colliders[j];
            if (!colA->Overlaps(colB)) continue;
            bool trigger = colA->IsTrigger() || colB->IsTrigger();
            if (trigger) {
                objA->notifyTriggerEnter(objB);
                objB->notifyTriggerEnter(objA);
            } else {
                objA->notifyCollisionEnter(objB);
                objB->notifyCollisionEnter(objA);
            }
        }
    }
}
