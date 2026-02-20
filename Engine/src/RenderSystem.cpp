#include "RenderSystem.h"
#include "Scene.h"
#include "object.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "Model3DComponent.h"
#include "IVoxelRenderer.h"
#include "image.h"
#include "text.h"
#include "Renderer.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void RenderSystem::Render(Scene *scene)
{
    if (!scene)
        return;

    // ── 1. Find active camera ────────────────────────────────────────
    CameraComponent *cam = CameraComponent::FindActive(scene);
    glm::mat4 view;
    glm::mat4 projection;
    if (cam)
    {
        view = cam->GetViewMatrix();
        projection = cam->GetProjectionMatrix();
    }
    else
    {
        // Fallback: default camera looking at origin
        view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));
        projection = glm::perspective(
            glm::radians(60.0f),
            static_cast<float>(Renderer::Get().GetWindowWidth()) /
                static_cast<float>(Renderer::Get().GetWindowHeight()),
            0.1f, 100.0f);
    }

    // ── 2. Find active light ─────────────────────────────────────────
    LightComponent *light = LightComponent::FindActive(scene);

    // ── 3. Shadow pass ───────────────────────────────────────────────
    if (light && light->IsShadowEnabled())
    {
        light->RenderShadowMap(scene);
    }

    // ── 4. Render all objects in layer order ─────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Camera position for distance culling
    glm::vec3 camPos(0.0f);
    if (cam)
    {
        glm::mat4 invView = glm::inverse(view);
        camPos = glm::vec3(invView[3]);
    }
    const float renderDistSq = 40.0f * 40.0f;

    // ── 4a. Voxel chunk meshes (3D, before UI) ───────────────────────
    if (IVoxelRenderer::s_instance)
    {
        glEnable(GL_DEPTH_TEST);
        IVoxelRenderer::s_instance->RenderChunks(view, projection, light);
    }

    const auto &objects = scene->GetObjects();
    for (auto *obj : objects)
    {
        if (!obj->IsActive())
            continue;

        // 3D model — depth test ON, with distance culling
        auto *model = obj->GetComponent<Model3DComponent>();
        if (model)
        {
            Vector3 p = obj->GetPosition3D();
            float dx = p.x - camPos.x;
            float dz = p.z - camPos.z;
            if (dx * dx + dz * dz <= renderDistSq)
            {
                glEnable(GL_DEPTH_TEST);
                model->Render(view, projection, light);
            }
            continue; // 3D objects don't carry Image/Text — skip those checks
        }

        // 2D sprite — depth test OFF (pure screen-space overlay)
        auto *image = obj->GetComponent<Image>();
        if (image)
        {
            glDisable(GL_DEPTH_TEST);
            image->Render();
        }

        // Text — depth test OFF
        auto *text = obj->GetComponent<TextComponent>();
        if (text)
        {
            glDisable(GL_DEPTH_TEST);
            text->Render();
        }
    }

    // Ensure depth test is on for next frame's shadow pass
    glEnable(GL_DEPTH_TEST);
}
