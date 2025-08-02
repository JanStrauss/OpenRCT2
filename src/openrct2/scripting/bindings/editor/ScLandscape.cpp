/*****************************************************************************
 * Copyright (c) 2025 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifdef ENABLE_SCRIPTING

    #include "ScLandscape.h"

    #include "../../../Context.h"
    #include "../../../world/map_generator/Erosion.h"
    #include "../../../world/map_generator/HeightMap.hpp"
    #include "../../../world/map_generator/SimplexNoise.h"

namespace OpenRCT2::Scripting
{
    std::shared_ptr<ScNoiseFn> ScLandscape::getSimplexNoiseFn(DukValue maybeSeed)
    {
        if (maybeSeed.type() == DukValue::NUMBER)
        {
            return std::make_shared<ScNoiseFn>(std::make_unique<World::MapGenerator::SimplexNoise>(maybeSeed.as_uint()));
        }
        return std::make_shared<ScNoiseFn>(std::make_unique<World::MapGenerator::SimplexNoise>());
    }

    std::shared_ptr<ScNoiseFn> ScLandscape::getSimplexFbmNoiseFn(
        float frequency, int32_t octaves, float lacunarity, float persistence, DukValue maybeSeed)
    {
        if (maybeSeed.type() == DukValue::NUMBER)
        {
            return std::make_shared<ScNoiseFn>(std::make_unique<World::MapGenerator::SimplexFbmNoise>(
                maybeSeed.as_uint(), frequency, octaves, lacunarity, persistence));
        }
        return std::make_shared<ScNoiseFn>(
            std::make_unique<World::MapGenerator::SimplexFbmNoise>(frequency, octaves, lacunarity, persistence));
    }

    std::vector<float> ScLandscape::simulateErosion(DukValue landscapeArgs)
    {
        auto dukCtx = GetContext()->GetScriptEngine().GetContext();

        auto tilesDuk = landscapeArgs["tiles"];
        auto genSettingsDuk = landscapeArgs["settings"];
        auto widthDuk = genSettingsDuk["sizeX"];
        auto heightDuk = genSettingsDuk["sizeY"];

        if (!tilesDuk.is_array()
            || genSettingsDuk.type() != DukValue::OBJECT
            || widthDuk.type() != DukValue::NUMBER
            || heightDuk.type() != DukValue::NUMBER)
        {
            duk_error(dukCtx, DUK_ERR_ERROR, "Invalid landscapeArgs");
        }

        auto tilesArray = tilesDuk.as_array();
        auto width = widthDuk.as_int();
        auto height = heightDuk.as_int();

        if (tilesArray.size() != static_cast<size_t>(width * height))
        {
            duk_error(dukCtx, DUK_ERR_ERROR, "tiles array size does not match width*height");
        }

        World::MapGenerator::ErosionSettings settings;
        World::MapGenerator::HeightMap<float> heightMap(width, height);

        // convert tile array to heightmap
        for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    auto tile = tilesArray[y * width + x];
                    auto level = tile["surfaceLevel"];

                    if (level.type() != DukValue::NUMBER)
                    {
                        continue;
                    }

                    heightMap[TileCoordsXY(x,y)] = level.as_float();
                }
        }

        World::MapGenerator::simulateErosion(settings, heightMap);

        std::vector<float> result(width * height);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                result[y * width + x] = heightMap[TileCoordsXY(x,y)];
            }
        }

        return result;
    }

    void ScLandscape::Register(duk_context* ctx)
    {
        dukglue_register_method(ctx, &ScLandscape::getSimplexNoiseFn, "getSimplexNoiseFn");
        dukglue_register_method(ctx, &ScLandscape::getSimplexFbmNoiseFn, "getSimplexFbmNoiseFn");
        dukglue_register_method(ctx, &ScLandscape::simulateErosion, "simulateErosion");
    }

} // namespace OpenRCT2::Scripting
#endif
