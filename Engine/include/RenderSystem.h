#pragma once

class Scene;

/// RenderSystem — centralised rendering orchestrator.
///
/// Called once per frame by the Engine, AFTER Scene::UpdateScene().
/// Responsibilities:
///   1. Find the active camera and light in the scene
///   2. Execute the shadow pass (via LightComponent)
///   3. Render all 3D models (Model3DComponent)
///   4. Render all 2D overlays (Image/Sprite, TextComponent)
///
/// This replaces the old approach where each component rendered itself
/// inside its own LateUpdate, duplicating camera/light lookup logic.
class RenderSystem {
public:
    /// Render the entire scene: shadows → 3D → 2D overlay.
    static void Render(Scene* scene);
};
