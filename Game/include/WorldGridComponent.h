#pragma once
#include "component.h"
#include "object.h"
#include "Scene.h"
#include "BlockComponent.h"
#include "CameraComponent.h"
#include <unordered_map>
#include <string>
#include <random>
#include <cmath>

class WorldGridComponent : public Component {
public:
	WorldGridComponent() : width(16), depth(16), blockSize(20.0f), originX(0.0f), originZ(0.0f), maxRenderDistance(50.0f), lastCullingTime(0.0f) {}

	void Init() override {}
	void Update(float dt) override {
		// Visibility culling disabled — small grid, render everything
	}

	void SetSize(int w, int d) { width = w; depth = d; }
	void SetBlockSize(float s) { blockSize = s; }
	void SetOrigin(float ox, float oz) { originX = ox; originZ = oz; }
	void SetMaxRenderDistance(float distance) { maxRenderDistance = distance; }

	float GetBlockSize() const { return blockSize; }

	void GenerateFlat(BlockType type) {
		if (!object) return;
		clearAll();
		for (int gz = 0; gz < depth; ++gz) {
			for (int gx = 0; gx < width; ++gx) {
				createBlock(gx, 0, gz, type);
			}
		}
	}

	void GenerateRandomColumns(int minH, int maxH, BlockType typeTop, BlockType typeBelow, unsigned int seed = std::random_device{}()) {
		if (!object) return;
		clearAll();
		std::mt19937 rng(seed);
		std::uniform_int_distribution<int> dist(minH, maxH);
		for (int gz = 0; gz < depth; ++gz) {
			for (int gx = 0; gx < width; ++gx) {
				int h = dist(rng);
				if (h < 1) h = 1;
				for (int gy = 0; gy < h; ++gy) {
					BlockType bt = (gy == h - 1) ? typeTop : typeBelow;
					createBlock(gx, gy, gz, bt);
				}
			}
		}
	}

	void GenerateHillyTerrain(int baseHeight, int maxHillHeight, BlockType surfaceType, BlockType undergroundType, unsigned int seed = std::random_device{}()) {
		if (!object) return;
		clearAll();
		std::mt19937 rng(seed);

		// Build a value-noise heightmap with proper spatial coherence
		std::vector<std::vector<int>> heightMap(width, std::vector<int>(depth, baseHeight));

		// Generate coarse random grid for value noise interpolation
		// Use a grid spacing so nearby cells share interpolated values
		const int gridStep = 4; // noise grid cell size
		int noiseW = (width  / gridStep) + 2;
		int noiseD = (depth  / gridStep) + 2;
		std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
		std::vector<std::vector<float>> noiseGrid(noiseW, std::vector<float>(noiseD));
		for (int i = 0; i < noiseW; ++i)
			for (int j = 0; j < noiseD; ++j)
				noiseGrid[i][j] = dist01(rng);

		// Bilinear interpolation of noise grid to produce smooth heightmap
		for (int gx = 0; gx < width; ++gx) {
			for (int gz = 0; gz < depth; ++gz) {
				float fx = (float)gx / gridStep;
				float fz = (float)gz / gridStep;
				int ix = (int)fx;
				int iz = (int)fz;
				float tx = fx - ix;
				float tz = fz - iz;
				// Smoothstep for smoother transitions
				tx = tx * tx * (3.0f - 2.0f * tx);
				tz = tz * tz * (3.0f - 2.0f * tz);

				float v00 = noiseGrid[ix][iz];
				float v10 = noiseGrid[ix + 1][iz];
				float v01 = noiseGrid[ix][iz + 1];
				float v11 = noiseGrid[ix + 1][iz + 1];
				float val = v00 * (1 - tx) * (1 - tz)
				          + v10 * tx * (1 - tz)
				          + v01 * (1 - tx) * tz
				          + v11 * tx * tz;

				int height = baseHeight + (int)(val * maxHillHeight);
				height = std::max(1, height);
				heightMap[gx][gz] = height;
			}
		}

		// Additional smoothing pass
		for (int gx = 1; gx < width - 1; ++gx) {
			for (int gz = 1; gz < depth - 1; ++gz) {
				int avg = (heightMap[gx-1][gz] + heightMap[gx+1][gz] + 
						   heightMap[gx][gz-1] + heightMap[gx][gz+1] + 
						   heightMap[gx][gz]) / 5;
				heightMap[gx][gz] = avg;
			}
		}

		// Create blocks from heightmap
		for (int gz = 0; gz < depth; ++gz) {
			for (int gx = 0; gx < width; ++gx) {
				int height = heightMap[gx][gz];
				for (int gy = 0; gy < height; ++gy) {
					BlockType bt = (gy == height - 1) ? surfaceType : undergroundType;
					createBlock(gx, gy, gz, bt);
				}
			}
		}
	}

	bool WorldToGrid(const Vector3& world, int& gx, int& gy, int& gz) const {
		float fx = (world.x - originX) / blockSize + width * 0.5f;
		float fz = (world.z - originZ) / blockSize + depth * 0.5f;
		float fy = world.y / blockSize;
		gx = (int)std::floor(fx + 0.5f);
		gz = (int)std::floor(fz + 0.5f);
		gy = (int)std::floor(fy + 0.5f);
		return gx >= 0 && gx < width && gz >= 0 && gz < depth && gy >= 0;
	}

	Vector3 GridToWorld(int gx, int gy, int gz) const {
		float wx = originX + (gx - width * 0.5f) * blockSize;
		float wy = gy * blockSize;
		float wz = originZ + (gz - depth * 0.5f) * blockSize;
		return Vector3(wx, wy, wz);
	}

	Object* GetBlock(int gx, int gy, int gz) const {
		std::string key = keyFor(gx, gy, gz);
		auto it = grid.find(key);
		return (it != grid.end()) ? it->second : nullptr;
	}

	Object* CreateBlockAt(int gx, int gy, int gz, BlockType type) {
		if (gx < 0 || gx >= width || gz < 0 || gz >= depth || gy < 0) return nullptr;
		// Don't place a block if one already exists here
		if (GetBlock(gx, gy, gz)) return nullptr;
		return createBlock(gx, gy, gz, type);
	}

	void RemoveBlockAt(int gx, int gy, int gz) {
		std::string key = keyFor(gx, gy, gz);
		auto it = grid.find(key);
		if (it != grid.end()) {
			if (it->second) {
				object->GetScene()->DeleteObject(it->second);
			}
			grid.erase(it);
		}
	}

	Object* GetBlock(int gx, int gz) const { return GetBlock(gx, 0, gz); }
	Object* CreateBlockAt(int gx, int gz, BlockType type) { return CreateBlockAt(gx, 0, gz, type); }
	void RemoveBlockAt(int gx, int gz) { RemoveBlockAt(gx, 0, gz); }

private:
	std::string keyFor(int gx, int gy, int gz) const {
		return std::to_string(gx) + ":" + std::to_string(gy) + ":" + std::to_string(gz);
	}

	Object* createBlock(int gx, int gy, int gz, BlockType type) {
		Object* b = CreateObject();
		b->AddComponent(new BlockComponent());
		if (auto* model = b->GetComponent<Model3DComponent>()) {
			model->SetSizeIsRelative(false);
		}
		b->GetComponent<BlockComponent>()->SetType(type);
		Vector3 pos = GridToWorld(gx, gy, gz);
		b->SetPosition(pos);
		b->SetSize(Vector3(1,1,1) * blockSize);
		std::string key = keyFor(gx, gy, gz);
		grid[key] = b;
		return b;
	}

	void clearAll() {
		for (auto& kv : grid) {
			if (kv.second) {
				object->GetScene()->DeleteObject(kv.second);
			}
		}
		grid.clear();
	}

	void updateVisibilityCulling() {
		if (!object || !object->GetScene()) return;
		
		Vector3 cameraPos = Vector3(0, 0, 0);
		const auto& objects = object->GetScene()->GetObjects();
		for (auto* obj : objects) {
			if (obj) {
				auto* cam = obj->GetComponent<CameraComponent>();
				if (cam && cam->IsActive()) {
					cameraPos = obj->GetPosition3D();
					break;
				}
			}
		}
		
		for (auto& kv : grid) {
			if (kv.second) {
				Vector3 blockPos = kv.second->GetPosition3D();
				Vector3 diff = blockPos - cameraPos;
				float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
				bool shouldBeVisible = distance <= maxRenderDistance * blockSize;
				
				kv.second->SetActive(shouldBeVisible);
			}
		}
	}

private:
	int width;
	int depth;
	float blockSize;
	float originX, originZ;
	float maxRenderDistance;
	float lastCullingTime;
	std::unordered_map<std::string, Object*> grid;
};
