#ifndef WORLD_HPP
#define WORLD_HPP


#include "Chunk.hpp"
#include "Camera.hpp"
#include "TerrainGenerator.hpp"
#include <vector>
#include <unordered_map>

//#define FRUSTUM_CULLING_ENABLED // currently broken? 

class World {
public:
	BlockDatabase blockdb;
	bool verticesAndIndicesUpdated = false;


	World() {}
	~World() {}

	void update(Camera& cam, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
		camPositionNew = cam.position;
		camChunkCoordsNew = getChunkXZ(cam.position);
		updateLoadList();

		// if there is a change in the camera or the frustum
		if (camPositionOld != camPositionNew || camFrustum != cam.getFrustum()) {
			// get the newest data
			camFrustum = cam.getFrustum();
			camPositionOld = camPositionNew;
		}

		// if the camera has crossed into a new chunk or a vertex update is being forced
		if (camChunkCoordsOld != camChunkCoordsNew || forceVertexUpdate) {
			camChunkCoordsOld = camChunkCoordsNew;
			updateVisibleList();
			updateRenderList();
			updateUnloadList();

			updateVerticesAndIndices(vertices, indices);
			forceVertexUpdate = false;
		}
	}

	static Vec4 getBlockXZ(Vec4 worldCoords) {
		return Vec4((int)worldCoords.x % AppGlobals::CHUNK_WIDTH, (int)worldCoords.y, (int)worldCoords.z % AppGlobals::CHUNK_WIDTH, (int)worldCoords.w);
	}

	static Vec4 getChunkXZ(Vec4 worldCoords) {
		return Vec4((int)worldCoords.x / AppGlobals::CHUNK_WIDTH, 0, (int)worldCoords.z / AppGlobals::CHUNK_WIDTH, (int)worldCoords.w);
	}

	BlockId getBlock(Vec4 blockPos) {
		auto blockPosition = getBlockXZ(blockPos);
		auto chunkPosition = getChunkXZ(blockPos);

		return getChunk(chunkPosition)->getBlock(blockPosition);
	}

	bool setBlock(BlockId id, Vec4 blockPos) {
		auto blockPosition = getBlockXZ(blockPos);
		auto chunkPosition = getChunkXZ(blockPos);

		if (getChunk(chunkPosition)->SetBlock(id, blockPosition)) {
			return true;
		}

		return false;
	}

	Chunk* getChunk(Vec4 chunkPos) {
		if (!chunkExistsAt(chunkPos)) {
			Chunk chunk(chunkPos);
			chunkMap.emplace(chunkPos, chunk);
		}

		return &chunkMap[chunkPos];
	}

	void initChunk(Vec4 chunkPos) {
		auto chunk = getChunk(chunkPos);

		// generate the terrain image
		auto image = terrainGenerator.GetTerrain(chunkPos.x, chunkPos.z);

		// sample the image at x and z coords to get y coord
		for (int x = 0; x < AppGlobals::CHUNK_WIDTH; x++) {
			for (int z = 0; z < AppGlobals::CHUNK_WIDTH; z++) {
				int y = image->GetValue(x, z).red;
				// set every block below the surface as well
				while (y >= 0) {
					chunk->SetBlock(BlockId::Grass, Vec4(x, y, z, 0));
					
					y--;
				}
			}
		}

		//// generate spheres of dirt lol
		//for (int z = 0; z < AppGlobals::CHUNK_WIDTH; z++) {
		//	for (int y = 0; y < AppGlobals::CHUNK_WIDTH; y++) {
		//		for (int x = 0; x < AppGlobals::CHUNK_WIDTH; x++) {
		//			if (sqrt((float)(x - AppGlobals::CHUNK_WIDTH / 2) * (x - AppGlobals::CHUNK_WIDTH / 2) + (y - AppGlobals::CHUNK_WIDTH / 2) * (y - AppGlobals::CHUNK_WIDTH / 2) + (z - AppGlobals::CHUNK_WIDTH / 2) * (z - AppGlobals::CHUNK_WIDTH / 2)) <= AppGlobals::CHUNK_WIDTH / 2) {
		//				chunk->SetBlock(BlockId::Grass, Vec4(x, y, z, 0));
		//			}
		//		}
		//	}
		//}
	}

	void updateChunk(Vec4 chunkPos) {
		generateVerticesAndIndices(chunkPos);
		forceVertexUpdate = true;
	}

private:
	TerrainGenerator terrainGenerator;
	std::vector<Vec4> chunkLoadList;
	std::vector<Vec4> visibleChunksList;
	std::vector<Vec4> renderableChunksList;
	std::vector<Vec4> chunkUnloadList;
	Vec4 camPositionOld;
	Vec4 camPositionNew;
	Vec4 camChunkCoordsOld;
	Vec4 camChunkCoordsNew;
	ViewFrustum camFrustum;

	std::unordered_map<Vec4, Chunk> chunkMap;
	bool forceVertexUpdate = false;


	void updateLoadList() {
		int numOfChunksLoaded = 0;

		// set bounds of how far out to render based on what chunk the player is in
		Vec4 lowChunkXZ(floor(camChunkCoordsNew.x) - AppGlobals::renderDistance, 0.f, floor(camChunkCoordsNew.z) - AppGlobals::renderDistance, 0.f);
		Vec4 highChunkXZ(floor(camChunkCoordsNew.x) + AppGlobals::renderDistance, 0.f, floor(camChunkCoordsNew.z) + AppGlobals::renderDistance, 0.f);

		// precompute squared render distance
		float sqRenderDistance = AppGlobals::renderDistance * AppGlobals::renderDistance;

		// for each chunk around the player
		for (float x = lowChunkXZ.x; x <= highChunkXZ.x; x++) {
			for (float z = lowChunkXZ.z; z <= highChunkXZ.z; z++) {
				Vec4 chunkPos(x, 0.f, z, 0.f);
				// if the chunk is in the view frustum (if frustum culling is enabled)
				#ifdef FRUSTUM_CULLING_ENABLED 
				if (camFrustum.isBoxInFrustum(c.bbox))
					#endif
				{
					// if the chunk is not already loaded
					if (!getChunk(chunkPos)->isLoaded) {
						// if the chunk is not already in the load list
						if (!ChunkAlreadyExistsIn(chunkLoadList, chunkPos)) {
							// put the chunk into the load list
							chunkLoadList.push_back(chunkPos);
						}
					}
				}
			}
		}

		// initialize each chunk in the load list if it is not already
		for (int i = 0; i < chunkLoadList.size(); i++) {
			auto chunk = getChunk(chunkLoadList[i]);

			if (!chunk->isInitialized) {
				initChunk(chunkLoadList[i]);
				chunk->isInitialized = true;
			}
		}

		// load the data for each chunk once they have all been initialized
		for (int i = 0; i < chunkLoadList.size(); i++) {
			if (numOfChunksLoaded != AppGlobals::asyncNumChunksPerFrame) {
				auto chunk = getChunk(chunkLoadList[i]);

				if (chunk->isInitialized) {
					// load the chunks data
					updateChunk(chunkLoadList[i]);

					// Increase the chunks loaded count
					numOfChunksLoaded++;

					// add the chunk to the visible list because it is potentially visible
					visibleChunksList.push_back(chunkLoadList[i]);

					// remove the chunk from the load list since it is now loaded
					chunkLoadList.erase(chunkLoadList.begin() + i);

					// subtract 1 from the index since the container size changed
					i--;
				}
			}
			else {
				//stop looping
				break;
			}
		}

	}

	void updateVisibleList() {
		// for each chunk in the potentially visible list
		for (int i = 0; i < visibleChunksList.size(); i++) {
			#ifdef FRUSTUM_CULLING_ENABLED 
			// if the chunk is in the view frustum (if frustum culling is enabled)
			if ( camFrustum.isBoxInFrustum(c.bbox)) 
			#endif
			{
				// if the chunk is loaded
				if (getChunk(visibleChunksList[i])->isLoaded) {
					// if the chunk is not already in the renderable list
					if (!ChunkAlreadyExistsIn(renderableChunksList, visibleChunksList[i])) {
						// add the chunk to the renderable chunk list because it is able to be seen by the player
						renderableChunksList.push_back(visibleChunksList[i]);

						// remove it from the visible list
						visibleChunksList.erase(visibleChunksList.begin() + i);

						// subtract 1 from the index since the container size changed
						i--;
					}
					// if the chunk is already in the renderable list
					else {
						// remove it from the visible list
						visibleChunksList.erase(visibleChunksList.begin() + i);

						// subtract 1 from the index since the container size changed
						i--;
					}
				}
				// if the chunk is in the frustum but not yet loaded
				else {
					// do nothing. ie: wait for the chunk to be loaded in the next few frames.
				}
			}
			#ifdef FRUSTUM_CULLING_ENABLED 
			// if the chunk is not in the frustum at all
			else {
				// remove the chunk from the visible list because it is not visible to the player and should not be rendered
				visibleChunksList.erase(visibleChunksList.begin() + i);

				// subtract 1 from the index since the container size changed
				i--;
			}
			#endif
		}
	}

	void updateRenderList() {
		// for each chunk in the render list
		for (int i = 0; i < renderableChunksList.size(); i++) {
			// if the distance to the chunk is greater than renderDistance on either axis
			auto xDistance = abs(camChunkCoordsNew.x - renderableChunksList[i].x);
			auto zDistance = abs(camChunkCoordsNew.z - renderableChunksList[i].z);
			if (xDistance > AppGlobals::renderDistance || zDistance > AppGlobals::renderDistance) {
				// add the chunk to the unload list because its out of render distance
				chunkUnloadList.push_back(renderableChunksList[i]);

				// remove the chunk from the renderable chunks list so it is no longer rendered
				renderableChunksList.erase(renderableChunksList.begin() + i);

				// subtract 1 from the index since the container size changed
				i--;
			}
		}
	}

	void updateUnloadList() {
		// for each chunk in the unload list
		for (int i = 0; i < chunkUnloadList.size(); i++) {
			// unload the chunk
			unLoadChunk(chunkUnloadList[i]);

			// remove the chunk from the unload list since it is now unloaded
			chunkUnloadList.erase(chunkUnloadList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
	}

	void unLoadChunk(Vec4 chunkPos) {
		// todo: Save chunk to file eventually
		if (chunkExistsAt(chunkPos)) {
			chunkMap.erase(chunkPos);
		}
	}

	bool chunkExistsAt(Vec4 chunkPos) {
		return chunkMap.find(chunkPos) != chunkMap.end();
	}

	double sqDistanceToChunk(Chunk& chunk) {
		return ((camChunkCoordsNew.x - chunk.position.x) * (camChunkCoordsNew.x - chunk.position.x)) + ((camChunkCoordsNew.z - chunk.position.z) * (camChunkCoordsNew.z - chunk.position.z));
	}

	bool ChunkAlreadyExistsIn(std::vector<Vec4> v, Vec4 elem) {
		// for each chunk in the list
		for (int i = 0; i < v.size(); i++) {
			// break if the chunk is in the load list
			if (v[i] == elem ) {
				return true;
				break;
			}
		}

		return false;
	}

	void generateVerticesAndIndices(Vec4 chunkPos) {
		auto chunk = getChunk(chunkPos);

		chunk->vertices.clear();
		chunk->indices.clear();

		for (float y = 0; y < AppGlobals::CHUNK_HEIGHT; y++) {
			for (float x = 0; x < AppGlobals::CHUNK_WIDTH; x++) {
				for (float z = 0; z < AppGlobals::CHUNK_WIDTH; z++) {

					// infer the block position using its coordinates
					Vec4 blockPosition(x, (float)y, z, 0.f);

					auto blockId = chunk->getBlock(blockPosition);

					// dont render air
					if (blockId == BlockId::Air) {
						continue;
					}


					auto getBlock = [&](float x, float y, float z) {
						auto cw = AppGlobals::CHUNK_WIDTH;

						// this stops layer 0 from always being rendered
						if (y == -1) {
							return BlockId::Grass;
						}

						// set bounds of how far out to render based on what chunk the player is in
						Vec4 lowChunkXZ(floor(camChunkCoordsNew.x) - AppGlobals::renderDistance, 0.f, floor(camChunkCoordsNew.z) - AppGlobals::renderDistance, 0.f);
						Vec4 highChunkXZ(floor(camChunkCoordsNew.x) + AppGlobals::renderDistance, 0.f, floor(camChunkCoordsNew.z) + AppGlobals::renderDistance, 0.f);

						auto otherChunk = getChunk(chunkPos);

						if ((x < 0 && chunk->position.x == lowChunkXZ.x) ||
							(z < 0 && chunk->position.z == lowChunkXZ.z) ||
							(x >= cw && chunk->position.x == highChunkXZ.x) ||
							(z >= cw && chunk->position.z == highChunkXZ.z)) {
							return BlockId::Grass;
						}



						if (x < 0 && z < 0) {
							x += cw;
							z += cw;
							otherChunk = getChunk(Vec4(chunkPos.x - 1, 0.f, chunkPos.z - 1, 0.f));
						}
						else if (x >= cw && z >= cw) {
							x -= cw;
							z -= cw;
							otherChunk = getChunk(Vec4(chunkPos.x + 1, 0.f, chunkPos.z + 1, 0.f));
						}
						else if (x < 0 && z >= cw) {
							x += cw;
							z -= cw;
							otherChunk = getChunk(Vec4(chunkPos.x - 1, 0.f, chunkPos.z + 1, 0.f));
						}
						else if (x >= cw && z < 0) {
							x -= cw;
							z += cw;
							otherChunk = getChunk(Vec4(chunkPos.x + 1, 0.f, chunkPos.z - 1, 0.f));
						}
						else if (x < 0) {
							x += cw;
							otherChunk = getChunk(Vec4(chunkPos.x - 1, 0.f, chunkPos.z, 0.f));
						}
						else if (x >= cw) {
							x -= cw;
							otherChunk = getChunk(Vec4(chunkPos.x + 1, 0.f, chunkPos.z, 0.f));
						}
						else if (z < 0) {
							z += cw;
							otherChunk = getChunk(Vec4(chunkPos.x, 0.f, chunkPos.z - 1, 0.f));
						}
						else if (z >= cw) {
							z -= cw;
							otherChunk = getChunk(Vec4(chunkPos.x, 0.f, chunkPos.z + 1, 0.f));
						}


						return otherChunk->getBlock(Vec4(x, y, z, 0.f));
					};

					if (getBlock(x + 1, y, z) == BlockId::Air ||
						getBlock(x - 1, y, z) == BlockId::Air ||
						getBlock(x, y + 1, z) == BlockId::Air ||
						getBlock(x, y - 1, z) == BlockId::Air ||
						getBlock(x, y, z + 1) == BlockId::Air ||
						getBlock(x, y, z - 1) == BlockId::Air) {
						// get the block's data
						auto verts = blockdb.blockDataFor(blockId).getVertices();
						auto inds = blockdb.blockDataFor(blockId).getIndices();

						// save the offset for the indices
						auto offset = chunk->vertices.size();

						// account for the block position and chunk position and store the new verts for later
						for (int i = 0; i < verts.size(); i++) {
							Vertex v(verts[i]);
							v.pos.x += blockPosition.x;
							v.pos.y += blockPosition.y;
							v.pos.z += blockPosition.z;
							v.pos.x += chunk->position.x * AppGlobals::CHUNK_WIDTH; // coords are now in world coords format. 
							v.pos.z += chunk->position.z * AppGlobals::CHUNK_WIDTH;
							chunk->vertices.push_back(v);
						}

						// account for the offset into vertices vector and store the indices for later
						for (int i = 0; i < inds.size(); i++) {
							unsigned int ind(inds[i] + offset);
							chunk->indices.push_back(ind);
						}
					}
				}
			}
		}
		assert(chunk->vertices.size() > 0);
		chunk->isLoaded = true;
	}

	void updateVerticesAndIndices(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
		vertices.clear();
		indices.clear();

		// for each chunk in the render list
		for (int i = 0; i < renderableChunksList.size(); i++) {
			auto chunk = getChunk(renderableChunksList[i]);

			if (!chunk->isLoaded) {
				initChunk(renderableChunksList[i]);
			}

			// get the chunks data
			auto verts = chunk->vertices;
			auto inds = chunk->indices;

			// save the offset for the indices
			auto offset = vertices.size();

			vertices.insert(vertices.end(), verts.begin(), verts.end());

			// account for the offset into the vertices vector and store the indices for later
			for (int i = 0; i < inds.size(); i++) {
				auto ind(inds[i] + offset);
				indices.push_back(ind);
			}
		}

		if (vertices.size() > 0 && indices.size() > 0) {
			verticesAndIndicesUpdated = true;
		}
	}
};
#endif // WORLD_HPP