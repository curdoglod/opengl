#include "Scene.h"
#include "object.h"
#include "engine.h"
#include "sprite.h"
#include "Renderer.h"
#include "BoxCollider3D.h"
#include <algorithm>

// ── Destructor ──────────────────────────────────────────────────────

Scene::~Scene() {
    for (auto* object : objects) {
        delete object;
    }
    objects.clear();
}

// ── Lifecycle ───────────────────────────────────────────────────────

void Scene::PreInit(Engine* engine, SDL_Window* window) {
    m_window = window;
    m_engine = engine;
    SetWindowSize((int)GetWindowSize().x, (int)GetWindowSize().y);
}

// ── Per-frame ───────────────────────────────────────────────────────

void Scene::UpdateEvents(SDL_Event event) {
    // Iterate over a snapshot so that handlers can safely
    // create / delete objects without invalidating the loop.
    auto snapshot = objects;
    for (auto& object : snapshot) {
        if (object->IsActive()) {
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

void Scene::UpdateScene(float deltaTime) {
    Update();
    // Pass 1: Logic update
    for (auto& object : objects) {
        if (object->IsActive()) {
            object->update(deltaTime);
        }
    }
    // Pass 2: Collision callbacks
    dispatchCollisions();
    // Pass 3: Late update (post-logic, NOT rendering — RenderSystem handles that)
    for (auto& object : objects) {
        if (object->IsActive()) {
            object->lateUpdate(deltaTime);
        }
    }
}

// ── Object management ───────────────────────────────────────────────

Object* Scene::CreateObject() {
    Object* object = new Object(this);
    objects.push_back(object);
    updateLayer();
    return object;
}

void Scene::DeleteObject(Object* object) {
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (*it == object) {
            delete *it;
            objects.erase(it);
            updateLayer();
            break;
        }
    }
}

Sprite* Scene::createSprite(const std::vector<unsigned char>& imageData) {
    return new Sprite(imageData);
}

void Scene::updateLayer() {
    std::sort(objects.begin(), objects.end(), [](const auto& a, const auto& b) {
        return a->GetLayer() < b->GetLayer();
    });
}

// ── Window / scene switching ────────────────────────────────────────

Vector2 Scene::GetWindowSize() {
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    return Vector2((float)w, (float)h);
}

void Scene::SwitchToScene(Scene* newScene) {
    m_engine->ChangeScene(newScene);
}

void Scene::PushScene(Scene* scene) {
    m_engine->PushScene(scene);
}

void Scene::PopScene() {
    m_engine->PopScene();
}

void Scene::SetWindowSize(const int& w, const int& h) {
    SDL_SetWindowSize(m_window, w, h);
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    Renderer::Get().SetWindowSize(w, h);
}

// ── Collision dispatch ──────────────────────────────────────────────

void Scene::dispatchCollisions()
{
    // Gather all active objects that carry a BoxCollider3D
    std::vector<std::pair<Object*, BoxCollider3D*>> colliders;
    for (auto* obj : objects) {
        if (!obj->IsActive()) continue;
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
