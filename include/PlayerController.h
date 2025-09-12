#pragma once
#include "component.h"
#include "Utils.h"
#include "Model3DComponent.h"

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
		Vector3 move(0,0,0);
		if (keyW) move.z -= 1;
		if (keyS) move.z += 1;
		if (keyA) move.x -= 1;
		if (keyD) move.x += 1;
		if (move.x != 0 || move.z != 0) {
			float len = std::sqrt(move.x*move.x + move.z*move.z);
			move.x /= len; move.z /= len;
			pos.x += move.x * moveSpeed * dt;
			pos.z += move.z * moveSpeed * dt;
			object->SetPosition(pos);
		}
		if (followCamera) {
			followCamera->SetPosition(pos + camOffset);
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

	// Camera follow
	void SetCamera(Object* cam) { followCamera = cam; }
	void SetCameraOffset(const Vector3& off) { camOffset = off; }
	void SetMoveSpeed(float s) { moveSpeed = s; }

	// Visual config
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

private:
	bool keyW, keyA, keyS, keyD;
	float moveSpeed;
	Object* followCamera;
	Vector3 camOffset;
	std::string modelPath;
	Vector3 modelSize;
};
