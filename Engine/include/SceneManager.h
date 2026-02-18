#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <stack>
#include <vector>
#include <memory>

class Scene;
class Engine;
struct SDL_Window;

/// SceneManager — manages a stack of scenes.
///
/// The *top* scene on the stack is the "active" one: it receives Update,
/// events, and rendering each frame.  Pushing a new scene pauses the
/// previous one (it stays alive on the stack).  Popping returns to the
/// scene below.  `ReplaceScene` is the old behaviour — it clears the
/// whole stack and switches to a brand-new scene.
///
/// Ownership: SceneManager owns every Scene* that is pushed onto it and
/// will `delete` them when they are popped or when the manager itself is
/// destroyed.
///
/// Deferred deletion: scenes are not destroyed immediately (a scene may
/// call SwitchToScene from its own callback).  Old scenes are kept in a
/// pending-delete list and cleaned up at the start of the next frame via
/// `FlushPending()`.
class SceneManager {
public:
    SceneManager() = default;
    ~SceneManager();

    // Non-copyable
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    /// Attach the engine + window so that every pushed scene can be
    /// initialised (PreInit / Awake / Init).
    void Bind(Engine* engine, SDL_Window* window);

    /// Replace the entire stack with a single new scene.
    /// This is equivalent to the old Engine::ChangeScene behaviour.
    /// Old scenes are deleted at the start of the next frame.
    void ReplaceScene(Scene* scene);

    /// Push a scene on top of the stack.  The previous scene stays alive
    /// but stops receiving updates.
    void PushScene(Scene* scene);

    /// Pop the top scene and resume the one below.
    /// The popped scene is deleted at the start of the next frame.
    /// Does nothing if the stack has only one scene.
    void PopScene();

    /// The currently active scene (top of the stack), or nullptr.
    Scene* GetActiveScene() const;

    /// True when at least one scene is on the stack.
    bool HasScene() const;

    /// Number of scenes on the stack.
    std::size_t SceneCount() const;

    /// Delete any scenes that were deferred from the previous frame.
    /// Called automatically by the engine at the start of each tick.
    void FlushPending();

private:
    void initScene(Scene* scene);
    void clearStack();

    std::stack<Scene*>   m_scenes;
    std::vector<Scene*>  m_pendingDelete;
    Engine*              m_engine = nullptr;
    SDL_Window*          m_window = nullptr;
};

#endif // SCENEMANAGER_H


