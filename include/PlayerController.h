#pragma once
#include "component.h"
#include "Utils.h"
#include "Model3DComponent.h"
#include "BlockComponent.h"
#include "WorldGridComponent.h"
#include <SDL.h>

class PlayerController : public Component {
public:
	PlayerController() : moveSpeed(120.0f), followCamera(nullptr), camOffset(0.0f, 6.0f, 10.0f) {
		keyW = keyA = keyS = keyD = false;
	}

	void Init() override {
		ensureModel();
	}
	
	void Update(float dt) override {
		if (!object) return;
		Vector3 pos = object->GetPosition3D();

		float vertical = (keyW ? 1.0f : 0.0f) + (keyS ? -1.0f : 0.0f);
		float horizontal = (keyD ? 1.0f : 0.0f) + (keyA ? -1.0f : 0.0f);
		if (vertical != 0.0f || horizontal != 0.0f) {
			const float toRad = 3.1415926535f / 180.0f;
			float cy = cosf(yaw * toRad);
			float sy = sinf(yaw * toRad);
			Vector3 forward(sy, 0.0f, -cy);
			Vector3 right(cy, 0.0f,  sy);
			Vector3 move = Vector3(
				forward.x * vertical + right.x * horizontal,
				0.0f,
				forward.z * vertical + right.z * horizontal
			);
			float len = std::sqrt(move.x*move.x + move.z*move.z);
			if (len > 0.0001f) {
				move.x /= len; move.z /= len;
				pos.x += move.x * moveSpeed * dt;
				pos.z += move.z * moveSpeed * dt;
				object->SetPosition(pos);
			}
		}

		if (followCamera) {
			followCamera->SetPosition(pos + camOffset);
			followCamera->SetRotation(Vector3(pitch, yaw, 0.0f));
		}
	}

	void onKeyPressed(SDL_Keycode key) override {
		if (key == SDLK_w) keyW = true;
		if (key == SDLK_a) keyA = true;
		if (key == SDLK_s) keyS = true;
		if (key == SDLK_d) keyD = true;
	}
	void onKeyReleased(SDL_Keycode key) override {
		if (key == SDLK_w) keyW = false;
		if (key == SDLK_a) keyA = false;
		if (key == SDLK_s) keyS = false;
		if (key == SDLK_d) keyD = false;
	}

	void OnMouseButtonMotion(Vector2 /*mouse_position*/) override {
		if (!followCamera) return;
		int dx = 0, dy = 0;
		SDL_GetRelativeMouseState(&dx, &dy);
		yaw   += dx * mouseSensitivity;
		pitch += dy * mouseSensitivity;
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;
	}

	void OnMouseButtonDown(Vector2 mouse_position) override {
		if (!object || !object->GetScene()) return;
		
		Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
		bool isLeftClick = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
		bool isRightClick = (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
		
		Vector3 rayStart = followCamera ? followCamera->GetPosition3D() : object->GetPosition3D();
		Vector3 rayDir = getLookDirection();
		
		WorldGridComponent* grid = nullptr;
		const auto& objects = object->GetScene()->GetObjects();
		for (auto* obj : objects) {
			grid = obj->GetComponent<WorldGridComponent>();
			if (grid) break;
		}
		
		if (!grid) return;
		
		if (isLeftClick) {
			Vector3 currentPos = rayStart;
			for (int i = 0; i < 50; ++i) {
				int gx, gy, gz;
				if (grid->WorldToGrid(currentPos, gx, gy, gz)) {
					Object* block = grid->GetBlock(gx, gy, gz);
					if (block) {
						grid->RemoveBlockAt(gx, gy, gz);
						break;
					}
				}
				currentPos = currentPos + rayDir * 2.0f;
			}
		}
		else if (isRightClick) {
			Vector3 currentPos = rayStart;
			Vector3 lastEmptyPos;
			bool foundEmpty = false;
			
			for (int i = 0; i < 50; ++i) {
				int gx, gy, gz;
				if (grid->WorldToGrid(currentPos, gx, gy, gz)) {
					Object* block = grid->GetBlock(gx, gy, gz);
					if (block) {
						if (foundEmpty) {
							int lgx, lgy, lgz;
							if (grid->WorldToGrid(lastEmptyPos, lgx, lgy, lgz)) {
								grid->CreateBlockAt(lgx, lgy, lgz, BlockType::Dirt);
							}
						}
						break;
					} else {
						lastEmptyPos = currentPos;
						foundEmpty = true;
					}
				}
				currentPos = currentPos + rayDir * 2.0f;
			}
		}
	}

	void SetCamera(Object* cam) { followCamera = cam; }
	void SetCameraOffset(const Vector3& off) { camOffset = off; }
	void SetMoveSpeed(float s) { moveSpeed = s; }
	void SetMouseSensitivity(float s) { mouseSensitivity = s; }

	void SetModelPath(const std::string& path) { modelPath = path; }
	void SetModelSize(const Vector3& size) { modelSize = size; }
	bool SetSkinTexture(const std::string& texturePath) {
		Model3DComponent* mc = object ? object->GetComponent<Model3DComponent>() : nullptr;
		if (!mc) return false;
		return mc->SetAlbedoTextureFromFile(texturePath);
	}

private:
	void ensureModel() {
		if (!object) return;
		Model3DComponent* mc = object->GetComponent<Model3DComponent>();
		if (!mc) {
			std::string path = modelPath.empty() ? std::string("Assets/cube.fbx") : modelPath;
			object->AddComponent(new Model3DComponent(path));
			object->AddComponent(new Rigidbody3D());
			object->AddComponent(new BoxCollider3D());
			mc = object->GetComponent<Model3DComponent>();
		}
		if (mc && (modelSize.x != 0 || modelSize.y != 0 || modelSize.z != 0)) {
			object->SetSize(modelSize);
		}
	}

	Vector3 getLookDirection() const {
		const float toRad = 3.1415926535f / 180.0f;
		float cy = cosf(yaw * toRad);
		float sy = sinf(yaw * toRad);
		float cp = cosf(pitch * toRad);
		float sp = sinf(pitch * toRad);
		return Vector3(sy * cp, -sp, -cy * cp);
	}

private:
	bool keyW, keyA, keyS, keyD;
	float moveSpeed;
	Object* followCamera;
	Vector3 camOffset;
	std::string modelPath;
	Vector3 modelSize;
	float yaw = 0.0f;
	float pitch = 0.0f;
	float mouseSensitivity = 0.15f;
};
