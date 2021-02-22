#pragma once
#ifndef NOISE_STATIC
#define NOISE_STATIC
#endif
#include "noise/noise.h"
#include "noiseutils.h"
#include <time.h>

class TerrainGenerator {
public:
	TerrainGenerator(){}

	module::Perlin myModule;
	utils::NoiseMap heightMap;
	utils::NoiseMapBuilderPlane heightMapBuilder;
	utils::RendererImage renderer;
	utils::Image image;
	utils::WriterBMP writer;
	

	utils::Image* GetTerrain(double chunkX, double chunkZ) {
		double scalar = static_cast<double>(25.0000000000);
		
		// define the boundaries of the part of the terrain we want to render
		double lowBoundX = chunkX / scalar;
		double highBoundX = (chunkX + 1) / scalar;
		double lowBoundZ = chunkZ / scalar;
		double highBoundZ = (chunkZ + 1) / scalar;

		// if there is no seed, create one from the current time
		if (AppGlobals::seed == -1) {
			AppGlobals::seed = time(NULL);
		}
		
		// apply the seed
		myModule.SetSeed(AppGlobals::seed);

		// create the heightmap
		heightMapBuilder.SetSourceModule(myModule);
		heightMapBuilder.SetDestNoiseMap(heightMap);
		heightMapBuilder.SetDestSize(AppGlobals::CHUNK_WIDTH, AppGlobals::CHUNK_WIDTH);
		heightMapBuilder.SetBounds(lowBoundX, highBoundX, lowBoundZ, highBoundZ);
		heightMapBuilder.Build();

		// render the heightmap to an image
		renderer.SetSourceNoiseMap(heightMap);
		renderer.SetDestImage(image);
		renderer.Render();

		// normalize the image to be within acceptable world coordinates. ie: no terrain above a threshold
		normalize(image);

		// optionally write it to a bmp file for debugging
		//writer.SetSourceImage(image);
		//writer.SetDestFilename("terrain.bmp");
		//writer.WriteDestFile();

		return &image;
	}


private:
	// converts the map's range of 0 to 255 into an acceptable range
	void normalize(utils::Image& img) {
		for (int x = 0; x < img.GetWidth(); x++) {
			for (int y = 0; y < img.GetHeight(); y++) {
				// double slope = 1.0 * (output_end - output_start) / (input_end - input_start)
				// output = output_start + round(slope * (input - input_start))

				float rmin = 0;
				float rmax = 255;
				unsigned char tmin = 0;
				unsigned char tmax = 127;
				float m = img.GetValue(x, y).red;

				float slope = 1 * (tmax - tmin) / (rmax - rmin);

				unsigned char newValue = tmin + floorf((slope * (m - rmin)) + 0.5f);
				utils::Color newColor(newValue, newValue, newValue, 255);
				
				img.SetValue(x, y, newColor);
			}
		}
	}
};

