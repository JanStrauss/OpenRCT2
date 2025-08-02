#pragma once

#ifdef ENABLE_SCRIPTING

    #include "HeightMap.hpp"

namespace OpenRCT2::World::MapGenerator
{

    struct ErosionSettings
    {
        uint32_t cycles = 200000;

        float density = 1.0;
        float evaporationRate = 0.001;
        float depositionRate = 0.1;
        float minVolume = 0.01;
        float friction = 0.05;
        float dt = 1.2f;
    };

    /**
     * Simulate hydraulic terrain erosion.
     *
     * Based on https://github.com/weigert/SimpleErosion
     *
     */
    void simulateErosion(ErosionSettings& settings, HeightMap<float>& heightMap);

} // namespace OpenRCT2::World::MapGenerator
#endif
