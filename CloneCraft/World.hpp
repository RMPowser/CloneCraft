#pragma once
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

	void update(Camera& cam, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
	{
		camNew = cam;
		camChunkCoordsNew = { camNew.position.x / AppGlobals::CHUNK_WIDTH, 0, camNew.position.z / AppGlobals::CHUNK_WIDTH, 0 };
		updateLoadList();

		// if there is a change in the camera or the frustum
		if (camOld != camNew || camFrustum != cam.getFrustum())
		{
			// get the newest data
			camFrustum = cam.getFrustum();
			camOld = camNew;
		}

		// if the camera has crossed into a new chunk or a vertex update is being forced
		if (camChunkCoordsOld.x != camChunkCoordsNew.x ||
			camChunkCoordsOld.z != camChunkCoordsNew.z ||
			forceVertexUpdate)
		{
			camChunkCoordsOld = camChunkCoordsNew;
			updateRenderList();
			updateUnloadList();

			updateVerticesAndIndices(vertices, indices);
			forceVertexUpdate = false;
		}
	}

	static GW::MATH::GVECTORF getBlockXZ(int x, int z)
	{
		return { static_cast<float>(x % AppGlobals::CHUNK_WIDTH), 0, static_cast<float>(z % AppGlobals::CHUNK_WIDTH), 0 };
	}

	static GW::MATH::GVECTORF getChunkXZ(int x, int z)
	{
		return { static_cast<float>(x / AppGlobals::CHUNK_WIDTH), 0, static_cast<float>(z / AppGlobals::CHUNK_WIDTH), 0 };
	}

	BlockId getBlock(int x, int y, int z)
	{
		auto blockPosition = getBlockXZ(x, z);
		auto chunkPosition = getChunkXZ(x, z);

		return getChunk(chunkPosition.x, chunkPosition.z).getBlock(blockPosition.x, y, blockPosition.z);
	}

	bool setBlock(BlockId id, int x, int y, int z)
	{
		auto blockPosition = getBlockXZ(x, z);
		auto chunkPosition = getChunkXZ(x, z);

		if (getChunk(chunkPosition.x, chunkPosition.z).setBlock(id, blockPosition.x, y, blockPosition.z))
		{
			return true;
		}

		return false;
	}

	Chunk& getChunk(int x, int z)
	{
		GW::MATH::GVECTORF key{ x, 0, z, 0 };
		if (!chunkExistsAt(x, z))
		{
			Chunk chunk(key);
			chunkMap.emplace(key, std::move(chunk));
		}

		return chunkMap[key];
	}

	void updateChunk(Chunk& chunk)
	{
		generateVerticesAndIndices(chunk);
		forceVertexUpdate = true;
	}

private:
	std::vector<Chunk> renderableChunksList;
	std::vector<Chunk> chunkLoadList;
	std::vector<Chunk> chunkUnloadList;
	Camera camOld;
	Camera camNew;
	GW::MATH::GVECTORF camChunkCoordsOld;
	GW::MATH::GVECTORF camChunkCoordsNew;
	ViewFrustum camFrustum;

	std::unordered_map<GW::MATH::GVECTORF, Chunk> chunkMap;
	bool forceVertexUpdate = false;


	void updateLoadList()
	{
		int numOfChunksLoaded = 0;

		// set bounds of how far out to render based on what chunk the player is in
		GW::MATH::GVECTORF lowChunkXZ = { camChunkCoordsNew.x - AppGlobals::renderDistance, 0, camChunkCoordsNew.z - AppGlobals::renderDistance, 0 };
		GW::MATH::GVECTORF highChunkXZ = { camChunkCoordsNew.x + AppGlobals::renderDistance, 0, camChunkCoordsNew.z + AppGlobals::renderDistance, 0 };

		// precompute squared render distance
		float sqRenderDistance = AppGlobals::renderDistance * AppGlobals::renderDistance;

		// for each chunk around the player
		for (float x = lowChunkXZ.x; x <= highChunkXZ.x; x++)
		{
			for (float z = lowChunkXZ.z; z <= highChunkXZ.z; z++)
			{
				// if the chunk is within render distance
				if (sqDistanceToChunk(getChunk(x, z)) < sqRenderDistance)
				{
					// if the chunk is in the view frustum (if frustum culling is enabled)
					#ifdef FRUSTUM_CULLING_ENABLED 
					if (camFrustum.isBoxInFrustum(c.bbox))
						#endif
					{
						// if the chunk is not already loaded
						if (!getChunk(x, z).isLoaded)
						{
							// if the chunk is not already in the load list
							if (!ChunkAlreadyExistsIn(chunkLoadList, getChunk(x, z)))
							{
								// put the chunk into the load list
								chunkLoadList.push_back(getChunk(x, z));
							}
						}
					}
				}
			}
		}

		// for each chunk in the load list
		for (int i = 0; i < chunkLoadList.size(); i++)
		{
			// if we havent hit the chunk load limit per frame
			if (numOfChunksLoaded != AppGlobals::asyncNumChunksPerFrame)
			{
				// load the chunk
				initChunk(chunkLoadList[i]);

				// Increase the chunks loaded count
				numOfChunksLoaded++;

				// if the chunk is not already in the renderable list
				if (!ChunkAlreadyExistsIn(renderableChunksList, chunkLoadList[i]))
				{
					// add the chunk to the renderable chunk list because it is able to be seen by the player
					renderableChunksList.push_back(chunkLoadList[i]);

					// remove the chunk from the load list since it is now loaded
					chunkLoadList.erase(chunkLoadList.begin() + i);

					// subtract 1 from the index since the container size changed
					i--;
				}
			}
			// if we have hit the chunk load limit per frame
			else
			{
				//stop looping
				break;
			}
		}
	}

	void updateRenderList()
	{
		// precompute squared render distance
		float sqRenderDistance = AppGlobals::renderDistance * AppGlobals::renderDistance;

		// for each chunk in the render list
		for (int i = 0; i < renderableChunksList.size(); i++)
		{
			// if the chunk is not within render distance
			if (sqDistanceToChunk(renderableChunksList[i]) > sqRenderDistance)
			{
				// add the chunk to the unload list because its out of render distance
				chunkUnloadList.push_back(renderableChunksList[i]);

				// remove the chunk from the renderable chunks list so it is no longer rendered
				renderableChunksList.erase(renderableChunksList.begin() + i);

				// subtract 1 from the index since the container size changed
				i--;
			}
		}
	}

	void updateUnloadList()
	{
		// for each chunk in the unload list
		for (int i = 0; i < chunkUnloadList.size(); i++)
		{
			// unload the chunk
			unLoadChunk(chunkUnloadList[i]);

			// remove the chunk from the unload list since it is now unloaded
			chunkUnloadList.erase(chunkUnloadList.begin() + i);

			// subtract 1 from the index since the container size changed
			i--;
		}
	}

	void initChunk(Chunk& chunk) 
	{
		// generate the terrain image
		TerrainGenerator generator;
		auto image = generator.GetTerrain(chunk.position.x, chunk.position.z);

		// sample the image at x and z coords to get y coord
		for (int x = 0; x < AppGlobals::CHUNK_WIDTH; x++)
		{
			for (int z = 0; z < AppGlobals::CHUNK_WIDTH; z++)
			{
				int y = image->GetValue(x, z).red;
				chunk.setBlock(BlockId::Grass, x, y, z);
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

		updateChunk(chunk);
	}

	void unLoadChunk(Chunk& chunk)
	{
		// todo: Save chunk to file eventually
		if (chunkExistsAt(chunk.position.x, chunk.position.z))
		{
			GW::MATH::GVECTORF key{ chunk.position.x, 0, chunk.position.z, 0 };
			chunkMap.erase(key);
		}
	}

	bool chunkExistsAt(int x, int z)
	{
		GW::MATH::GVECTORF key{ static_cast<int>(x), 0, static_cast<int>(z), 0 };
		return chunkMap.find(key) != chunkMap.end();
	}

	double sqDistanceToChunk(Chunk& chunk)
	{
		return ((camChunkCoordsNew.x - chunk.position.x) * (camChunkCoordsNew.x - chunk.position.x)) + ((camChunkCoordsNew.z - chunk.position.z) * (camChunkCoordsNew.z - chunk.position.z));
	}

	bool ChunkAlreadyExistsIn(std::vector<Chunk>& v, Chunk& elem) 
	{
		// for each chunk in the list
		for (int i = 0; i < v.size(); i++)
		{
			// break if the chunk is in the load list
			if (v[i].position.x == elem.position.x && v[i].position.z == elem.position.z)
			{
				return true;
				break;
			}
		}

		return false;
	}

	void generateVerticesAndIndices(Chunk& chunk)
	{
		chunk.vertices.clear();
		chunk.indices.clear();

		for (float y = 0; y < AppGlobals::CHUNK_HEIGHT; y++)
		{
			for (float x = 0; x < AppGlobals::CHUNK_WIDTH; x++)
			{
				for (float z = 0; z < AppGlobals::CHUNK_WIDTH; z++)
				{
					// for each block in this chunk
					auto blockId = chunk.getBlock(x, y, z);

					// dont render air
					if (blockId == BlockId::Air)
					{
						continue;
					}

					// infer the block position using its coordinates
					GW::MATH::GVECTORF blockPosition = { x, y, z, 0 };

					// get the block's data
					auto verts = blockdb.blockDataFor(blockId).getVertices();
					auto inds = blockdb.blockDataFor(blockId).getIndices();

					// save the offset for the indices
					auto offset = chunk.vertices.size();

					// account for the block position and chunk position and store the new verts for later
					for (int i = 0; i < verts.size(); i++)
					{
						Vertex v(verts[i]);
						v.pos.x += blockPosition.x;
						v.pos.y += blockPosition.y;
						v.pos.z += blockPosition.z;
						v.pos.x += chunk.position.x * AppGlobals::CHUNK_WIDTH; // coords are now in world coords format. 
						v.pos.z += chunk.position.z * AppGlobals::CHUNK_WIDTH;
						chunk.vertices.push_back(v);
					}

					// account for the offset into vertices vector and store the indices for later
					for (int i = 0; i < inds.size(); i++)
					{
						unsigned int ind(inds[i] + offset);
						chunk.indices.push_back(ind);
					}
				}
			}
		}
		assert(chunk.vertices.size() > 0);
		chunk.isLoaded = true;
	}

	void updateVerticesAndIndices(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) 
	{
		vertices.clear();
		indices.clear();

		// for each chunk in the render list
		for (int i = 0; i < renderableChunksList.size(); i++)
		{
			if (!renderableChunksList[i].isLoaded)
			{
				initChunk(renderableChunksList[i]);
			}

			// get the chunks data
			auto verts = renderableChunksList[i].vertices;
			auto inds = renderableChunksList[i].indices;

			// save the offset for the indices
			auto offset = vertices.size();

			vertices.insert(vertices.end(), verts.begin(), verts.end());

			// account for the offset into the vertices vector and store the indices for later
			for (int i = 0; i < inds.size(); i++)
			{
				auto ind(inds[i] + offset);
				indices.push_back(ind);
			}
		}

		if (vertices.size() > 0 && indices.size() > 0)
		{
			verticesAndIndicesUpdated = true;
		}
	}
};