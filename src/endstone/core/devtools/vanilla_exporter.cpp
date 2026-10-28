// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "endstone/core/devtools/vanilla_exporter.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

#include <zstr.hpp>

#include "bedrock/nbt/nbt_io.h"
#include "endstone/core/devtools/jigsaw_packet_capture.h"
#include "bedrock/util/string_byte_output.h"

namespace endstone::core::devtools {
namespace {

void saveJson(const std::filesystem::path &path, const nlohmann::json &value)
{
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    file << value;
    if (!file.good()) {
        throw std::runtime_error("Unable to write " + path.string());
    }
}

void saveNbt(const std::filesystem::path &path, const CompoundTag &value)
{
    std::string buffer;
    BigEndianStringByteOutput output(buffer);
    NbtIo::writeNamedTag("", value, output);

    zstr::ofstream file(path.string(), std::ios::out | std::ios::binary | std::ios::trunc);
    file << buffer;
    if (!file.good()) {
        throw std::runtime_error("Unable to write " + path.string());
    }
}

}  // namespace

std::optional<std::filesystem::path> exportJigsawPacket(const std::filesystem::path &base_path)
{
    auto captured = JigsawPacketCapture::snapshot();
    if (!captured) {
        return std::nullopt;
    }

    std::error_code error;
    std::filesystem::create_directories(base_path, error);
    if (error) {
        throw std::runtime_error("Unable to create " + base_path.string() + ": " + error.message());
    }

    const auto path = base_path / "jigsaw_structure_data_packet.bin";
    std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
    file.write(captured->data(), static_cast<std::streamsize>(captured->size()));
    if (!file.good()) {
        throw std::runtime_error("Unable to write " + path.string());
    }
    return path;
}

std::vector<std::filesystem::path> exportAll(const std::filesystem::path &base_path, const VanillaData &data)
{
    std::error_code error;
    std::filesystem::create_directories(base_path, error);
    if (error) {
        throw std::runtime_error("Unable to create " + base_path.string() + ": " + error.message());
    }

    std::vector<std::filesystem::path> files;
    files.reserve(11);

    auto save_json = [&](const nlohmann::json &value, const char *name) {
        auto path = base_path / name;
        saveJson(path, value);
        files.emplace_back(std::move(path));
    };

    auto save_nbt = [&](const CompoundTag &value, const char *name) {
        auto path = base_path / name;
        saveNbt(path, value);
        files.emplace_back(std::move(path));
    };

    save_json(data.block_types, "block_types.json");
    save_json(data.block_states, "block_states.json");
    save_json(data.block_tags, "block_tags.json");
    save_json(data.items, "items.json");
    save_json(data.item_tags, "item_tags.json");
    save_json(data.creative_groups, "creative_groups.json");
    save_json(data.biomes, "biomes.json");

    save_nbt(data.item_components, "item_components.nbt");

    CompoundTag block_palette;
    block_palette.put("blocks", data.block_palette.copy());
    save_nbt(block_palette, "block_palette.nbt");

    CompoundTag creative_items;
    creative_items.put("items", data.creative_items.copy());
    save_nbt(creative_items, "creative_items.nbt");

    nlohmann::json recipes = {
        {"shapeless", data.recipes.shapeless},
        {"shaped", data.recipes.shaped},
        {"furnace", data.recipes.furnace},
        {"furnaceAux", data.recipes.furnace_aux},
        {"multi", data.recipes.multi},
        {"userDataShapeless", data.recipes.user_data_shapeless},
        {"shapelessChemistry", data.recipes.shapeless_chemistry},
        {"shapedChemistry", data.recipes.shaped_chemistry},
        {"smithingTransform", data.recipes.smithing_transform},
        {"smithingTrim", data.recipes.smithing_trim},
        {"potionMixes", data.recipes.potion_mixes},
        {"containerMixes", data.recipes.container_mixes},
        {"materialReducer", data.recipes.material_reducer},
    };
    save_json(recipes, "recipes.json");

    if (auto jigsaw_packet = exportJigsawPacket(base_path)) {
        files.emplace_back(*jigsaw_packet);
    }

    return files;
}

}  // namespace endstone::core::devtools
