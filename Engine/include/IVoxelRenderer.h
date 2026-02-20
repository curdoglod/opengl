#pragma once
// ---------------------------------------------------------------------------
//  IVoxelRenderer — abstract interface for batch-rendered voxel chunk meshes.
//
//  Implemented by WorldGridComponent (in Game/).  Registered as a global
//  singleton so that RenderSystem and LightComponent (in Engine/) can call
//  the render methods without depending on Game code.
// ---------------------------------------------------------------------------

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Frustum.h"

class LightComponent;

class IVoxelRenderer {
public:
    virtual ~IVoxelRenderer() = default;

    /// Render every loaded chunk mesh (main colour pass).
    virtual void RenderChunks(const glm::mat4& view,
                              const glm::mat4& projection,
                              LightComponent* light,
                              const Frustum& frustum) = 0;

    /// Render every loaded chunk mesh into the shadow depth buffer.
    virtual void RenderChunksDepth(GLuint depthProgram,
                                   const glm::mat4& lightVP,
                                   const Frustum& lightFrustum) = 0;

    /// Global instance — set by the active voxel world component.
    static inline IVoxelRenderer* s_instance = nullptr;  // C++17
};
