/*****************************************************************************
 * Copyright (c) 2025 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifdef ENABLE_SCRIPTING

    #include "Erosion.h"

    #include "../../Context.h"
    #include "../../Diagnostic.h"
    #include "../../GameState.h"

    #include <numbers>

namespace OpenRCT2::World::MapGenerator
{
    using namespace std::numbers;

    constexpr uint32_t PRINT_EVERY_N_CYCLES = 1000;

    constexpr float WEIGHT_CARDINAL =  0.15;
    constexpr float WEIGHT_ORDINAL =  0.1;

    struct Particle
    {
        Particle(FloatCoordsXY _position)
        {
            position = _position;
        }
        FloatCoordsXY position;
        FloatCoordsXY speed = FloatCoordsXY(0.0, 0.0);
        float volume = 1.0;
        float sediment = 0.0;
    };

    /**
     * Compute the surface normal at heightMap[x,y] by comparing against the neighbor tile heights.
     * The original impl in https://github.com/weigert/SimpleErosion uses float vecs and the more common Y-height convention.
     * Uses the OpenRCT Z-height convention.
     */
    static FloatCoordsXYZ surfaceNormal(HeightMap<float>& heightMap, int32_t x, int32_t y)
    {
        auto normal = FloatCoordsXYZ(0, 0, 0);

        normal += FloatCoordsXYZ(heightMap.get(x, y) - heightMap.get(x + 1, y), 1.0, 0.0).Normalize() * WEIGHT_CARDINAL;
        normal += FloatCoordsXYZ(heightMap.get(x - 1, y) - heightMap.get(x, y), 1.0, 0.0).Normalize() * WEIGHT_CARDINAL;
        normal += FloatCoordsXYZ(0.0, 1.0, heightMap.get(x, y) - heightMap.get(x, y + 1)).Normalize() * WEIGHT_CARDINAL;
        normal += FloatCoordsXYZ(0.0, 1.0, heightMap.get(x, y - 1) - heightMap.get(x, y)).Normalize() * WEIGHT_CARDINAL;

        normal += FloatCoordsXYZ((heightMap.get(x, y) - heightMap.get(x + 1, y + 1)) / sqrt2, sqrt2, (heightMap.get(x, y) - heightMap.get(x + 1, y + 1)) / sqrt2).Normalize() * WEIGHT_ORDINAL;
        normal += FloatCoordsXYZ((heightMap.get(x, y) - heightMap.get(x + 1, y - 1)) / sqrt2, sqrt2, (heightMap.get(x, y) - heightMap.get(x + 1, y - 1)) / sqrt2).Normalize() * WEIGHT_ORDINAL;
        normal += FloatCoordsXYZ((heightMap.get(x, y) - heightMap.get(x - 1, y + 1)) / sqrt2, sqrt2, (heightMap.get(x, y) - heightMap.get(x - 1, y + 1)) / sqrt2).Normalize() * WEIGHT_ORDINAL;
        normal += FloatCoordsXYZ((heightMap.get(x, y) - heightMap.get(x - 1, y - 1)) / sqrt2, sqrt2, (heightMap.get(x, y) - heightMap.get(x - 1, y - 1)) / sqrt2).Normalize() * WEIGHT_ORDINAL;
        
        return normal;
    }

    void simulateErosion(ErosionSettings& settings, HeightMap<float>& heightMap)
    {
        for (uint32_t i = 0; i < settings.cycles; ++i)
        {
            if (i % PRINT_EVERY_N_CYCLES == 0)
            {
                LOG_INFO("Erosion cycle %i/%i", i, settings.cycles);
            }

            auto particle = Particle(FloatCoordsXY(
                    1 + ScenarioRandMax(heightMap.width - 2),
                    1 + ScenarioRandMax(heightMap.height - 2)));

            while (particle.volume > settings.minVolume)
            {
                auto initialPosition = particle.position.AsTileCoordsXY();
                auto normal = surfaceNormal(heightMap, initialPosition.x, initialPosition.y);
                
                particle.speed += FloatCoordsXY{ normal.x, normal.z } / (particle.volume * settings.density);
                particle.position += particle.speed  * settings.dt;
                particle.speed *= 1.0 - settings.dt * settings.friction * particle.volume;

                auto updatedPosition = particle.position.AsTileCoordsXY();

                if (updatedPosition.x < 1
                    || updatedPosition.y < 1
                    || updatedPosition.x >= heightMap.width - 1
                    || updatedPosition.y >= heightMap.height - 1)
                {
                    break;
                }

                auto heightDiff = heightMap[initialPosition] - heightMap[updatedPosition];
                auto sedimentMax = std::max(0.0f, particle.volume * particle.speed.Length() * heightDiff);
                auto sedimentDiff = sedimentMax - particle.sediment;

                particle.sediment += settings.dt * settings.depositionRate * sedimentDiff;
                heightMap[initialPosition] -= settings.dt * particle.volume * settings.depositionRate * sedimentDiff;
                particle.volume *= 1.0 - settings.dt * settings.evaporationRate;
            }
        }
    }
} // namespace OpenRCT2::World::MapGenerator
#endif
