#pragma once
// ---------------------------------------------------------------------------
//  Frustum — 6-plane frustum extracted from a View*Projection matrix.
//
//  Usage:
//      Frustum f;
//      f.Extract(projection * view);
//      if (f.TestSphere(center, radius)) { /* visible */ }
//      if (f.TestAABB(aabbMin, aabbMax)) { /* visible */ }
// ---------------------------------------------------------------------------

#include <glm/glm.hpp>

struct Frustum {
    glm::vec4 planes[6]; // left, right, bottom, top, near, far

    /// Build the 6 frustum planes from a combined VP matrix.
    /// Uses the Gribb-Hartmann column extraction method.
    void Extract(const glm::mat4& vp) {
        // Each row of the transposed VP gives us a clip-space inequality.
        // plane[i] = (a, b, c, d)  ->  ax + by + cz + d >= 0  means "inside".
        for (int i = 0; i < 4; ++i) {
            planes[0][i] = vp[i][3] + vp[i][0]; // left
            planes[1][i] = vp[i][3] - vp[i][0]; // right
            planes[2][i] = vp[i][3] + vp[i][1]; // bottom
            planes[3][i] = vp[i][3] - vp[i][1]; // top
            planes[4][i] = vp[i][3] + vp[i][2]; // near
            planes[5][i] = vp[i][3] - vp[i][2]; // far
        }
        // Normalise so distance tests give real-world units.
        for (int i = 0; i < 6; ++i) {
            float len = glm::length(glm::vec3(planes[i]));
            if (len > 0.0f) planes[i] /= len;
        }
    }

    /// Returns true if the sphere is at least partially inside the frustum.
    bool TestSphere(const glm::vec3& center, float radius) const {
        for (int i = 0; i < 6; ++i) {
            if (glm::dot(glm::vec3(planes[i]), center) + planes[i].w < -radius)
                return false;
        }
        return true;
    }

    /// Returns true if the AABB is at least partially inside the frustum.
    bool TestAABB(const glm::vec3& mn, const glm::vec3& mx) const {
        for (int i = 0; i < 6; ++i) {
            // Pick the AABB corner that is *most* in the direction of the plane normal
            glm::vec3 p;
            p.x = (planes[i].x >= 0.0f) ? mx.x : mn.x;
            p.y = (planes[i].y >= 0.0f) ? mx.y : mn.y;
            p.z = (planes[i].z >= 0.0f) ? mx.z : mn.z;
            if (glm::dot(glm::vec3(planes[i]), p) + planes[i].w < 0.0f)
                return false;
        }
        return true;
    }
};
