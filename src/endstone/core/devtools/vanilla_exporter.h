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

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "endstone/core/devtools/vanilla_data.h"

class Level;

namespace endstone::core::devtools {

/**
 * Exports all vanilla data used by Allay to the specified directory.
 *
 * The caller must ensure that VanillaData has already been collected. The
 * function creates the destination directory when necessary and throws
 * std::runtime_error when a file cannot be written.
 */
std::vector<std::filesystem::path> exportAll(const std::filesystem::path &base_path, const VanillaData &data);

/**
 * Exports the complete original network packet (including the varint header).
 * Returns nullopt if BDS has not sent packet ID 313 to a client yet.
 */
std::optional<std::filesystem::path> exportJigsawPacket(const std::filesystem::path &base_path);

}  // namespace endstone::core::devtools
