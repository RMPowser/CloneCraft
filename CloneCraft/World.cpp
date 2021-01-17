#pragma once
#include "World.h"
#include "Chunk.h"
#ifndef NOISE_STATIC
#define NOISE_STATIC
#endif // !NOISE_STATIC
#include "noise/noise.h"
#include "noiseutils.h"

//#define FRUSTUM_CULLING_ENABLED // currently broken 


std::unordered_map<Vec2XZ, Chunk> chunkMap;
bool forceVertexUpdate = false;


World::World(BlockDatabase& _blockdb, AppConfig& _config) :
	terrainGenerator(CHUNK_WIDTH),
	seed(_config.seed) {
	blockdb = _blockdb;
	renderDistance = _config.renderDistance;
	maxChunksPerFrame = _config.asyncNumChunksPerFrame;
}

void World::update(Camera& cam, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
	camPositionNew = cam.position;
	camChunkCoordsNew = { (int)camPositionNew.x / CHUNK_WIDTH, (int)camPositionNew.z / CHUNK_WIDTH };
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

void World::updateLoadList() {
	int numOfChunksLoaded = 0;

	// set bounds of how far out to render based on what chunk the player is in
	Vec2XZ lowChunkXZ = { camChunkCoordsNew.x - renderDistance, camChunkCoordsNew.z - renderDistance };
	Vec2XZ highChunkXZ = { camChunkCoordsNew.x + renderDistance, camChunkCoordsNew.z + renderDistance };

	// for each chunk around the player within render distance
	for (int x = lowChunkXZ.x; x <= highChunkXZ.x; x++) {
		for (int z = lowChunkXZ.z; z <= highChunkXZ.z; z++) {
			Vec2XZ chunkXZ{ x, z };

			// if the chunk is not already loaded
			if (!getChunk(x, z).isLoaded) {
				// if the chunk is not already in the load list
				if (!ChunkAlreadyExistsIn(chunkLoadList, chunkXZ)) {
					// put the chunk into the load list
					chunkLoadList.push_back(chunkXZ);
				}
			}
		}
	}

	// for each chunk in the load list
	for (int i = 0; i < chunkLoadList.size(); i++) {
		// if we havent hit the chunk load limit per frame
		if (numOfChunksLoaded != maxChunksPerFrame) {
			Vec2XZ chunk{ chunkLoadList[i].x, chunkLoadList[i].z };

			// load the chunk
			loadChunk(chunk);
			forceVertexUpdate = true;

			// Increase the chunks loaded count
			numOfChunksLoaded++;

			// add the chunk to the visible list because it is potentially visible
			visibleChunksList.push_back(chunk);

			// remove the chunk from the load list since it is now loaded
			chunkLoadList.erase(chunkLoadList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
		// if we have hit the chunk load limit per frame
		else {
			//stop looping
			break;
		}
	}
}

void World::updateVisibleList() {
	// for each chunk in the potentially visible list
	for (int i = 0; i < visibleChunksList.size(); i++) {
		Vec2XZ chunkXZ{ visibleChunksList[i].x, visibleChunksList[i].z };

		// get the chunk at that location
		Chunk* chunk = &getChunk(chunkXZ);

		// if the chunk is in the view frustum (if frustum culling is enabled)
		if (
			#ifdef FRUSTUM_CULLING_ENABLED 
				camFrustum.isBoxInFrustum(chunk->bbox)
			#else
				true
			#endif
			) {
			// if the chunk is loaded
			if (chunk->isLoaded) {
				// if the chunk is not already in the renderable list
				if (!ChunkAlreadyExistsIn(renderableChunksList, chunkXZ)) {
					// add the chunk to the renderable chunk list because it is able to be seen by the player
					renderableChunksList.push_back(chunkXZ);

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
		// if the chunk is not in the frustum at all
		else {
			// remove the chunk from the visible list because it is not visible to the player and should not be rendered
			visibleChunksList.erase(visibleChunksList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
	}

	//printf("visibleListSize: %zd \n", visibleChunksList.size());
}

void World::updateRenderList() {
	// for each chunk in the render list
	for (int i = 0; i < renderableChunksList.size(); i++) {
		Vec2XZ chunk{ renderableChunksList[i].x, renderableChunksList[i].z };

		// if the distance to the chunk is greater than renderDistance on either axis
		auto xDistance = abs(camChunkCoordsNew.x - chunk.x);
		auto zDistance = abs(camChunkCoordsNew.z - chunk.z);
		if (xDistance > renderDistance || zDistance > renderDistance) {
			// add the chunk to the unload list because its out of render distance
			chunkUnloadList.push_back(chunk);

			// remove the chunk from the renderable chunks list so it is no longer rendered
			renderableChunksList.erase(renderableChunksList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
	}
}

void World::updateUnloadList() {
	// for each chunk in the unload list
	for (int i = 0; i < chunkUnloadList.size(); i++) {
		Vec2XZ chunk{ chunkUnloadList[i].x, chunkUnloadList[i].z };

		// if the chunk is currently loaded and the distance to the chunk is greater than the renderDistance on either axis
		if (getChunk(chunk).isLoaded) {
			// unload the chunk
			unLoadChunk(chunk);

			// remove the chunk from the unload list since it is now unloaded
			chunkUnloadList.erase(chunkUnloadList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
	}
	
	//printf("unloadListSize: %zd\n", chunkUnloadList.size());
}





Vec2XZ World::getBlockXZ(int x, int z) {
	return { x % CHUNK_WIDTH, z % CHUNK_WIDTH };
}

Vec2XZ World::getChunkXZ(int x, int z) {
	return { x / CHUNK_WIDTH, z / CHUNK_WIDTH };
}

BlockId World::getBlock(int x, int y, int z) {
	auto blockPosition = getBlockXZ(x, z);
	auto chunkPosition = getChunkXZ(x, z);

	return getChunk(chunkPosition.x, chunkPosition.z).getBlock(blockPosition.x, y, blockPosition.z);
}

bool World::setBlock(BlockId id, int x, int y, int z) {
	auto blockPosition = getBlockXZ(x, z);
	auto chunkPosition = getChunkXZ(x, z);

	if (getChunk(chunkPosition.x, chunkPosition.z).setBlock(id, blockPosition.x, y, blockPosition.z)) {
		return true;
	}

	return false;
}

Chunk& World::getChunk(int x, int z) {
	Vec2XZ key{ x, z };
	if (!chunkExistsAt(x, z)) {
		Chunk chunk(*this, Vec2XZ{ x, z });
		chunkMap.emplace(key, std::move(chunk));
	}

	return chunkMap[key];
}

Chunk& World::getChunk(Vec2XZ chunkPos) {
	if (!chunkExistsAt(chunkPos.x, chunkPos.z)) {
		Chunk chunk(*this, Vec2XZ{ chunkPos.x, chunkPos.z });
		chunkMap.emplace(chunkPos, std::move(chunk));
	}

	return chunkMap[chunkPos];
}

void World::loadChunk(Vec2XZ chunkPos) {
	Chunk* chunk = &getChunk(chunkPos.x, chunkPos.z);
	
	// generate the terrain image
	auto image = terrainGenerator.GetTerrain(chunkPos.x, chunkPos.z, seed);
	
	// sample the image at x and z coords to get y coord
	for (int x = 0; x < CHUNK_WIDTH; x++) {
		for (int z = 0; z < CHUNK_WIDTH; z++) {
			int y = image->GetValue(x, z).red;
			chunk->setBlock(BlockId::Grass, x, y, z);
		}
	}

	// generate spheres of dirt....
	//for (int z = 0; z < CHUNK_WIDTH; z++) {
	//	for (int y = 0; y < CHUNK_WIDTH; y++) {
	//		for (int x = 0; x < CHUNK_WIDTH; x++) {
	//			if (sqrt((float)(x - CHUNK_WIDTH / 2) * (x - CHUNK_WIDTH / 2) + (y - CHUNK_WIDTH / 2) * (y - CHUNK_WIDTH / 2) + (z - CHUNK_WIDTH / 2) * (z - CHUNK_WIDTH / 2)) <= CHUNK_WIDTH / 2) {
	//				chunk->setBlock(BlockId::Grass, x, y, z);
	//			}
	//		}
	//	}
	//}

	chunk->generateVerticesAndIndices();

	chunk->isLoaded = true;
}

void World::updateChunk(Vec2XZ chunkPos) {
	Chunk* chunk = &getChunk(chunkPos);

	chunk->generateVerticesAndIndices();
	forceVertexUpdate = true;
}

void World::unLoadChunk(Vec2XZ chunkPos) {
	// todo: Save chunk to file eventually
	if (chunkExistsAt(chunkPos.x, chunkPos.z)) {
		Vec2XZ key{ chunkPos.x, chunkPos.z };
		chunkMap.erase(key);
	}
}

bool World::chunkExistsAt(int x, int z) {
	Vec2XZ key{ x, z };
	return chunkMap.find(key) != chunkMap.end();
}

double World::distanceToChunk(Vec2XZ chunkXZ) {
	return sqrt(((camChunkCoordsNew.x - chunkXZ.x) * (camChunkCoordsNew.x - chunkXZ.x)) + ((camChunkCoordsNew.z - chunkXZ.z) * (camChunkCoordsNew.z - chunkXZ.z)));
}

bool World::ChunkAlreadyExistsIn(std::vector<Vec2XZ> v, Vec2XZ elem) {
	// for each chunk in the list
	for (int i = 0; i < v.size(); i++) {
		// break if the chunk is in the load list
		if (v[i] == elem) {
			return true;
			break;
		}
	}

	return false;
}

void World::updateVerticesAndIndices(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
	vertices.clear();
	indices.clear();

	// for each chunk in the render list
	for (int i = 0; i < renderableChunksList.size(); i++) {
		Vec2XZ chunkXZ{ renderableChunksList[i].x, renderableChunksList[i].z };
		// get the chunk
		Chunk* chunk = &getChunk(chunkXZ);

		/*if (!chunk->isVisible) {
			break;
		}*/

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

	verticesAndIndicesUpdated = true;
}