#pragma once
#include "component.h"
#include "object.h"
#include "Scene.h"
#include "Utils.h"
#include "BlockComponent.h"
#include "WorldGridComponent.h"
#include "CameraComponent.h"
#include "InputManager.h"
#include <SDL.h>
#include <cmath>
#include <algorithm>

// ============================================================================
// PlayerController — first-person camera controller (Minecraft-style).
//
// The owning Object is an invisible "body" that only carries position and a
// collider (no visible model).  The attached CameraComponent's owning Object
// is moved to match the player's eye position every frame.
//
// Responsibilities:
//   • WASD movement relative to camera yaw (horizontal only)
//   • Mouse-look (yaw / pitch)
//   • Simple gravity + ground collision via the world grid
//   • Block place / destroy raycasting on mouse click
// ============================================================================

class PlayerController : public Component {
public:
	PlayerController()
		: moveSpeed(120.0f / 35.0f)
		, cameraObject(nullptr)
		, eyeHeight(30.0f / 35.0f)
		, yaw(0.0f)
		, pitch(0.0f)
		, mouseSensitivity(0.15f)
		, velocityY(0.0f)
		, gravity(-600.0f / 35.0f)
		, isGrounded(false)
		, jumpSpeed(220.0f / 35.0f)
	{}

	// --- Setup helpers (call before adding to Object) -----------------------

	void SetCamera(Object* cam)          { cameraObject = cam; }
	void SetMoveSpeed(float s)           { moveSpeed = s; }
	void SetMouseSensitivity(float s)    { mouseSensitivity = s; }
	void SetEyeHeight(float h)           { eyeHeight = h; }
	void SetJumpSpeed(float s)           { jumpSpeed = s; }
	void SetGravity(float g)             { gravity = g; }

	// --- Component interface ------------------------------------------------

	void Init() override {
		// The player object should NOT have a visible model.
		// It only needs a position in the world.
	}

	void Update(float dt) override {
		if (!object) return;

		auto& input = InputManager::Get();

		// ---- Mouse look ----------------------------------------------------
		Vector2 mouseDelta = input.GetMouseDelta();
		yaw   += mouseDelta.x * mouseSensitivity;
		pitch += mouseDelta.y * mouseSensitivity;
		pitch = std::max(-89.0f, std::min(89.0f, pitch));

		Vector3 pos = object->GetPosition3D();
		WorldGridComponent* grid = findGrid();

		// ---- Push out of any block we are stuck inside ---------------------
		if (grid) {
			pushOutOfBlocks(grid, pos);
		}

		// ---- Horizontal movement (WASD relative to yaw) --------------------
		float vertical   = (input.IsKeyDown(SDLK_w) ? 1.0f : 0.0f) + (input.IsKeyDown(SDLK_s) ? -1.0f : 0.0f);
		float horizontal = (input.IsKeyDown(SDLK_d) ? 1.0f : 0.0f) + (input.IsKeyDown(SDLK_a) ? -1.0f : 0.0f);

		if (vertical != 0.0f || horizontal != 0.0f) {
			const float toRad = 3.1415926535f / 180.0f;
			float cy = cosf(yaw * toRad);
			float sy = sinf(yaw * toRad);

			Vector3 forward(sy, 0.0f, -cy);
			Vector3 right  (cy, 0.0f,  sy);

			Vector3 move(
				forward.x * vertical + right.x * horizontal,
				0.0f,
				forward.z * vertical + right.z * horizontal
			);
			float len = std::sqrt(move.x * move.x + move.z * move.z);
			if (len > 0.0001f) {
				move.x /= len;
				move.z /= len;

				// Try X and Z movement separately so the player can slide
				// along walls instead of getting stuck completely.
				float dx = move.x * moveSpeed * dt;
				float dz = move.z * moveSpeed * dt;

				if (grid) {
					// Try moving along X
					Vector3 testX(pos.x + dx, pos.y, pos.z);
					if (!isCollidingHorizontally(grid, testX)) {
						pos.x += dx;
					}
					// Try moving along Z
					Vector3 testZ(pos.x, pos.y, pos.z + dz);
					if (!isCollidingHorizontally(grid, testZ)) {
						pos.z += dz;
					}
				} else {
					pos.x += dx;
					pos.z += dz;
				}
			}
		}

		// ---- Vertical movement (gravity + jump) ----------------------------
		if (input.IsKeyDown(SDLK_SPACE) && isGrounded) {
			velocityY = jumpSpeed;
			isGrounded = false;
		}

		velocityY += gravity * dt;
		pos.y += velocityY * dt;

		// ---- Ground / ceiling collision against the world grid --------------
		if (grid) {
			float groundY = getGroundLevel(grid, pos);
			if (pos.y <= groundY) {
				pos.y = groundY;
				velocityY = 0.0f;
				isGrounded = true;
			} else {
				isGrounded = false;
			}
			// Ceiling collision: if jumping into a block above, stop.
			float headY = pos.y + eyeHeight;
			int gx, gy, gz;
			if (grid->WorldToGrid(Vector3(pos.x, headY, pos.z), gx, gy, gz)) {
				if (grid->GetBlock(gx, gy, gz)) {
					Vector3 blockWorld = grid->GridToWorld(gx, gy, gz);
					float blockBottom = blockWorld.y - grid->GetBlockSize() * 0.5f;
					pos.y = blockBottom - eyeHeight;
					if (velocityY > 0.0f) velocityY = 0.0f;
				}
			}
		}

		object->SetPosition(pos);

		// ---- Sync camera to player eye position ----------------------------
		if (cameraObject) {
			cameraObject->SetPosition(Vector3(pos.x, pos.y + eyeHeight, pos.z));
			cameraObject->SetRotation(Vector3(pitch, yaw, 0.0f));
		}
	}

	// --- Input events -------------------------------------------------------

	void OnMouseButtonDown(Vector2 /*mouse_position*/) override {
		if (!object || !object->GetScene()) return;

		auto& clickInput = InputManager::Get();
		bool isLeftClick  = clickInput.IsMouseButtonDown(SDL_BUTTON_LEFT);
		bool isRightClick = clickInput.IsMouseButtonDown(SDL_BUTTON_RIGHT);

		WorldGridComponent* grid = findGrid();
		if (!grid) return;

		Vector3 rayStart = cameraObject ? cameraObject->GetPosition3D()
		                                : object->GetPosition3D();
		Vector3 rayDir   = getLookDirection();
		float   stepSize = grid->GetBlockSize() * 0.4f;

		if (isLeftClick) {
			// Destroy block
			Vector3 currentPos = rayStart;
			for (int i = 0; i < 40; ++i) {
				int gx, gy, gz;
				if (grid->WorldToGrid(currentPos, gx, gy, gz)) {
					if (grid->GetBlock(gx, gy, gz)) {
						grid->RemoveBlockAt(gx, gy, gz);
						break;
					}
				}
				currentPos = currentPos + rayDir * stepSize;
			}
		} else if (isRightClick) {
			// Place block
			Vector3 currentPos = rayStart;
			Vector3 lastEmptyPos;
			bool foundEmpty = false;
			for (int i = 0; i < 40; ++i) {
				int gx, gy, gz;
				if (grid->WorldToGrid(currentPos, gx, gy, gz)) {
					if (grid->GetBlock(gx, gy, gz)) {
						if (foundEmpty) {
							int lgx, lgy, lgz;
							if (grid->WorldToGrid(lastEmptyPos, lgx, lgy, lgz)) {
								grid->CreateBlockAt(lgx, lgy, lgz, BlockType::Grass);
							}
						}
						break;
					} else {
						lastEmptyPos = currentPos;
						foundEmpty = true;
					}
				}
				currentPos = currentPos + rayDir * stepSize;
			}
		}
	}

private:
	// --- Helpers ------------------------------------------------------------

	/// Compute the unit look direction from yaw + pitch.
	Vector3 getLookDirection() const {
		const float toRad = 3.1415926535f / 180.0f;
		float cy = cosf(yaw * toRad);
		float sy = sinf(yaw * toRad);
		float cp = cosf(pitch * toRad);
		float sp = sinf(pitch * toRad);
		return Vector3(sy * cp, -sp, -cy * cp);
	}

	/// Find the WorldGridComponent in the scene (cached after first lookup).
	WorldGridComponent* findGrid() {
		if (cachedGrid) return cachedGrid;
		if (!object || !object->GetScene()) return nullptr;
		const auto& objects = object->GetScene()->GetObjects();
		for (auto* obj : objects) {
			auto* g = obj->GetComponent<WorldGridComponent>();
			if (g) { cachedGrid = g; return g; }
		}
		return nullptr;
	}

	/// Get the Y world coordinate of the top of the highest block column
	/// under the player's feet.
	float getGroundLevel(WorldGridComponent* grid, const Vector3& pos) const {
		int gx, gy, gz;
		if (!grid->WorldToGrid(pos, gx, gy, gz)) {
			return 0.0f;
		}
		// Scan downward from current grid Y to find the top solid block
		for (int y = gy; y >= 0; --y) {
			if (grid->GetBlock(gx, y, gz)) {
				// Top of this block
				Vector3 blockWorld = grid->GridToWorld(gx, y, gz);
				return blockWorld.y + grid->GetBlockSize() * 0.5f;
			}
		}
		return 0.0f; // no ground found — fall to Y=0
	}

	/// Check whether the player body (a thin vertical column) overlaps any
	/// solid block at the given position.  We check at feet level and at
	/// mid-body level so the player cannot walk through one-block-high walls
	/// or two-block-high walls.
	bool isCollidingHorizontally(WorldGridComponent* grid, const Vector3& pos) const {
		float bs = grid->GetBlockSize();
		float halfBody = bs * 0.3f; // player body half-width (~60% of a block)

		// Heights to test: feet + small offset, and mid-body
		float testHeights[] = { pos.y + 1.0f, pos.y + eyeHeight * 0.5f };

		// Test four horizontal offsets (approximate the body as a small square)
		float offsets[][2] = {
			{ halfBody,  0.0f },
			{-halfBody,  0.0f },
			{ 0.0f,  halfBody },
			{ 0.0f, -halfBody }
		};

		for (float h : testHeights) {
			for (auto& off : offsets) {
				int gx, gy, gz;
				Vector3 probe(pos.x + off[0], h, pos.z + off[1]);
				if (grid->WorldToGrid(probe, gx, gy, gz)) {
					if (grid->GetBlock(gx, gy, gz)) {
						return true;
					}
				}
			}
			// Also check the center
			int gx, gy, gz;
			if (grid->WorldToGrid(Vector3(pos.x, h, pos.z), gx, gy, gz)) {
				if (grid->GetBlock(gx, gy, gz)) {
					return true;
				}
			}
		}
		return false;
	}

	/// If the player's center is inside a solid block, push them out to the
	/// nearest free side so they don't get permanently stuck.
	void pushOutOfBlocks(WorldGridComponent* grid, Vector3& pos) const {
		float bs = grid->GetBlockSize();
		// Check at feet level (slightly above)
		float checkY = pos.y + 1.0f;
		int gx, gy, gz;
		if (!grid->WorldToGrid(Vector3(pos.x, checkY, pos.z), gx, gy, gz))
			return;
		if (!grid->GetBlock(gx, gy, gz))
			return;

		// Player center is inside block (gx, gy, gz). Find nearest empty
		// neighbor and push toward it.
		Vector3 blockCenter = grid->GridToWorld(gx, gy, gz);
		float dx = pos.x - blockCenter.x;
		float dz = pos.z - blockCenter.z;

		// Determine which face is closest for pushout
		float absDx = std::abs(dx);
		float absDz = std::abs(dz);
		float halfBlock = bs * 0.5f;
		float halfBody = bs * 0.3f;

		if (absDx >= absDz) {
			// Push along X
			if (dx >= 0) {
				pos.x = blockCenter.x + halfBlock + halfBody + 0.1f;
			} else {
				pos.x = blockCenter.x - halfBlock - halfBody - 0.1f;
			}
		} else {
			// Push along Z
			if (dz >= 0) {
				pos.z = blockCenter.z + halfBlock + halfBody + 0.1f;
			} else {
				pos.z = blockCenter.z - halfBlock - halfBody - 0.1f;
			}
		}
	}

private:
	// Movement
	float moveSpeed;
	float velocityY;
	float gravity;
	float jumpSpeed;
	bool  isGrounded;

	// Camera
	Object* cameraObject;
	float   eyeHeight;
	float   yaw;
	float   pitch;
	float   mouseSensitivity;

	// Cached references
	WorldGridComponent* cachedGrid = nullptr;
};
