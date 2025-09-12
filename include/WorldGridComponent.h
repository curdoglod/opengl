#pragma once
#include "component.h"
#include "BlockComponent.h"
#include <unordered_map>
#include <string>
#include <random>

class WorldGridComponent : public Component {
public:
	WorldGridComponent() : width(16), depth(16), blockSize(20.0f), originX(0.0f), originZ(0.0f) {}

	void Init() override {}
	void Update(float dt) override {}

	void SetSize(int w, int d) { width = w; depth = d; }
	void SetBlockSize(float s) { blockSize = s; }
	void SetOrigin(float ox, float oz) { originX = ox; originZ = oz; }

	float GetBlockSize() const { return blockSize; }

	// Generate a flat ground at y=0 spanning width x depth
	void GenerateFlat(BlockType type) {
		if (!object) return;
		clearAll();
		for (int gz = 0; gz < depth; ++gz) {
			for (int gx = 0; gx < width; ++gx) {
				createBlock(gx, 0, gz, type);
			}
		}
	}

	// Random columns: for each (x,z) create a stack of blocks with random height [minH,maxH]
	// top block typeTop, below fill with typeBelow
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

	// Generate solid terrain with small hills using noise-like algorithm
	void GenerateHillyTerrain(int baseHeight, int maxHillHeight, BlockType surfaceType, BlockType undergroundType, unsigned int seed = std::random_device{}()) {
		if (!object) return;
		clearAll();
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> noiseDist(0.0f, 1.0f);
		
		// Generate height map with simple noise
		std::vector<std::vector<int>> heightMap(width, std::vector<int>(depth, baseHeight));
		
		// Add hills using multiple octaves of noise
		for (int gx = 0; gx < width; ++gx) {
			for (int gz = 0; gz < depth; ++gz) {
				float noise = 0.0f;
				float amplitude = 1.0f;
				float frequency = 0.1f;
				
				// Multiple octaves for more natural hills
				for (int octave = 0; octave < 3; ++octave) {
					float x = gx * frequency;
					float z = gz * frequency;
					noise += amplitude * (noiseDist(rng) * 2.0f - 1.0f);
					amplitude *= 0.5f;
					frequency *= 2.0f;
				}
				
				// Convert noise to height
				int height = baseHeight + (int)(noise * maxHillHeight);
				height = std::max(1, std::min(height, baseHeight + maxHillHeight));
				heightMap[gx][gz] = height;
			}
		}
		
		// Smooth the height map
		for (int gx = 1; gx < width - 1; ++gx) {
			for (int gz = 1; gz < depth - 1; ++gz) {
				int avg = (heightMap[gx-1][gz] + heightMap[gx+1][gz] + 
						   heightMap[gx][gz-1] + heightMap[gx][gz+1] + 
						   heightMap[gx][gz]) / 5;
				heightMap[gx][gz] = avg;
			}
		}
		
		// Create blocks based on height map
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

	// Convert world position -> grid indices (y derived as nearest layer)
	bool WorldToGrid(const Vector3& world, int& gx, int& gy, int& gz) const {
		float fx = (world.x - originX) / blockSize + width * 0.5f;
		float fz = (world.z - originZ) / blockSize + depth * 0.5f;
		float fy = world.y / blockSize;
		gx = (int)std::round(fx);
		gz = (int)std::round(fz);
		gy = (int)std::round(fy);
		return gx >= 0 && gx < width && gz >= 0 && gz < depth && gy >= 0;
	}

	// Convert grid indices -> world position at center of block
	Vector3 GridToWorld(int gx, int gy, int gz) const {
		float wx = originX + (gx - width * 0.5f) * blockSize;
		float wy = gy * blockSize;
		float wz = originZ + (gz - depth * 0.5f) * blockSize;
		return Vector3(wx, wy, wz);
	}

	// Block operations (3D)
	Object* GetBlock(int gx, int gy, int gz) const {
		std::string key = keyFor(gx, gy, gz);
		auto it = grid.find(key);
		return (it != grid.end()) ? it->second : nullptr;
	}

	Object* CreateBlockAt(int gx, int gy, int gz, BlockType type) {
		if (gx < 0 || gx >= width || gz < 0 || gz >= depth || gy < 0) return nullptr;
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

	// Backward-compatible 2D helpers (y=0)
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
		// Force absolute sizing to avoid model-dimension drift
		if (auto* model = b->GetComponent<Model3DComponent>()) {
			model->SetSizeIsRelative(false);
		}
		b->GetComponent<BlockComponent>()->SetType(type);
		// Position and scale snapped to grid
		Vector3 pos = GridToWorld(gx, gy, gz);
		b->SetPosition(Vector3(std::round(pos.x), std::round(pos.y), std::round(pos.z)));
		b->SetSize(Vector3(1,1,1) * (blockSize / 35.0f));
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

private:
	int width;
	int depth;
	float blockSize;
	float originX, originZ;
	std::unordered_map<std::string, Object*> grid;
};
