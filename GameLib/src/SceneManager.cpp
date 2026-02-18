#include "SceneManager.h"
#include "Scene.h"
#include "engine.h"

SceneManager::~SceneManager() {
    clearStack();
    FlushPending();
}

void SceneManager::Bind(Engine* engine, SDL_Window* window) {
    m_engine = engine;
    m_window = window;
}

void SceneManager::ReplaceScene(Scene* scene) {
    // Move every scene on the stack to the pending-delete list.
    // They will be destroyed at the start of the next frame,
    // so it's safe to call this from inside a scene callback.
    while (!m_scenes.empty()) {
        m_pendingDelete.push_back(m_scenes.top());
        m_scenes.pop();
    }
    initScene(scene);
    m_scenes.push(scene);
}

void SceneManager::PushScene(Scene* scene) {
    initScene(scene);
    m_scenes.push(scene);
}

void SceneManager::PopScene() {
    if (m_scenes.size() <= 1)
        return;                     // never pop the last scene

    m_pendingDelete.push_back(m_scenes.top());
    m_scenes.pop();
}

Scene* SceneManager::GetActiveScene() const {
    return m_scenes.empty() ? nullptr : m_scenes.top();
}

bool SceneManager::HasScene() const {
    return !m_scenes.empty();
}

std::size_t SceneManager::SceneCount() const {
    return m_scenes.size();
}

void SceneManager::FlushPending() {
    for (Scene* s : m_pendingDelete)
        delete s;
    m_pendingDelete.clear();
}

// ── private helpers ──────────────────────────────────────────────

void SceneManager::initScene(Scene* scene) {
    scene->PreInit(m_engine, m_window);
    scene->Awake();
    scene->Init();
}

void SceneManager::clearStack() {
    while (!m_scenes.empty()) {
        delete m_scenes.top();
        m_scenes.pop();
    }
}
