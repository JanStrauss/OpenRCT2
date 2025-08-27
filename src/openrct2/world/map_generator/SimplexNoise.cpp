/*****************************************************************************
 * Copyright (c) 2014-2025 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "SimplexNoise.h"

#include "../../util/Util.h"
#include "HeightMap.hpp"
#include "MapGen.h"
#include "MapHelpers.h"

#include <algorithm>
#include <fastnoiselite.hpp>

namespace OpenRCT2::World::MapGenerator
{
    static FastNoiseLite noise;

    void NoiseRand()
    {
        noise.SetSeed(UtilRand());
    }

    float FractalNoise(int32_t x, int32_t y, float frequency, int32_t octaves, float lacunarity, float persistence)
    {
        noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
        noise.SetFractalType(FastNoiseLite::FractalType_FBm);

        noise.SetFrequency(frequency);
        noise.SetFractalOctaves(octaves);
        noise.SetFractalLacunarity(lacunarity);
        noise.SetFractalGain(persistence);

        return noise.GetNoise(static_cast<float>(x),static_cast<float>(y));
    }

    /**
     * Smooths the height map.
     */
    static void smoothHeightMap(int32_t iterations, HeightMap& heightMap)
    {
        for (auto i = 0; i < iterations; i++)
        {
            auto copyHeight = heightMap;
            for (auto y = 1; y < heightMap.height - 1; y++)
            {
                for (auto x = 1; x < heightMap.width - 1; x++)
                {
                    auto avg = 0;
                    for (auto yy = -1; yy <= 1; yy++)
                    {
                        for (auto xx = -1; xx <= 1; xx++)
                        {
                            avg += copyHeight[{ y + yy, x + xx }];
                        }
                    }
                    avg /= 9;
                    heightMap[{ x, y }] = avg;
                }
            }
        }
    }

    static void generateSimplexNoise(Settings* settings, HeightMap& heightMap)
    {
        float freq = settings->simplex_base_freq / 100.0f * (1.0f / heightMap.width);
        int32_t octaves = settings->simplex_octaves;

        int32_t low = settings->heightmapLow / 2;
        int32_t high = settings->heightmapHigh / 2 - low;

        NoiseRand();
        for (int32_t y = 0; y < heightMap.height; y++)
        {
            for (int32_t x = 0; x < heightMap.width; x++)
            {
                float noiseValue = std::clamp(FractalNoise(x, y, freq, octaves, 2.0f, 0.65f), -1.0f, 1.0f);
                float normalisedNoiseValue = (noiseValue + 1.0f) / 2.0f;

                heightMap[{ x, y }] = low + static_cast<int32_t>(normalisedNoiseValue * high);
            }
        }
    }

    void generateSimplexMap(Settings* settings)
    {
        resetSurfaces(settings);

        // Create the temporary height map and initialise
        const auto& mapSize = settings->mapSize;
        const auto density = 2;
        auto heightMap = HeightMap(mapSize.x, mapSize.y, density);

        generateSimplexNoise(settings, heightMap);
        smoothHeightMap(2 + (UtilRand() % 6), heightMap);

        // Set the game map to the height map
        setMapHeight(settings, heightMap);

        if (settings->smoothTileEdges)
        {
            // Set the tile slopes so that there are no cliffs
            smoothMap(settings->mapSize, smoothTileStrong);
        }

        // Add the water
        setWaterLevel(settings->waterLevel);
    }
} // namespace OpenRCT2::World::MapGenerator
