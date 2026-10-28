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

#include "endstone/core/command/defaults/export_vanilla_command.h"

#include <atomic>
#include <filesystem>
#include <stdexcept>

#include "endstone/core/devtools/vanilla_data.h"
#include "endstone/core/devtools/vanilla_exporter.h"
#include "endstone/core/devtools/jigsaw_packet_capture.h"
#include "endstone/core/scheduler/scheduler.h"
#include "endstone/core/server.h"

namespace endstone::core {
namespace {
std::atomic_bool gExportInProgress = false;
}

ExportVanillaCommand::ExportVanillaCommand() : EndstoneCommand("exportvanilla")
{
    setDescription("Exports vanilla BDS data without the graphical DevTools.");
    setUsages("/exportvanilla");
    setAliases("exportdata");
    setPermissions("endstone.command.exportvanilla");
}

bool ExportVanillaCommand::execute(CommandSender &sender, const std::vector<std::string> &args) const
{
    if (!testPermission(sender)) {
        return true;
    }

    if (!sender.asConsole()) {
        sender.sendErrorMessage("This command can only be executed from the console.");
        return true;
    }

    if (gExportInProgress.exchange(true)) {
        sender.sendErrorMessage("A vanilla data export is already in progress.");
        return true;
    }

    auto output_path = std::filesystem::current_path() / "data";
    sender.sendMessage("Vanilla data export scheduled. Output directory: {}", output_path.string());

    auto &server = EndstoneServer::getInstance();
    auto &scheduler = static_cast<EndstoneScheduler &>(server.getScheduler());
    scheduler.runTask([output_path]() {
        auto &logger = EndstoneServer::getInstance().getLogger();
        try {
            logger.info("Collecting vanilla data from BDS...");
            auto *data = devtools::VanillaData::collect();
            if (!data) {
                throw std::runtime_error("The BDS level is not ready yet. Wait for 'Server started' and try again.");
            }

            logger.info("Writing vanilla data to {}...", output_path.string());
            auto files = devtools::exportAll(output_path, *data);
            if (!devtools::JigsawPacketCapture::hasPacket()) {
                logger.warning("Jigsaw packet 313 has not been captured. Connect a 1.26.60 client and rerun exportvanilla.");
            }
            for (const auto &file : files) {
                logger.info("Exported {}", file.filename().string());
            }
            logger.info("Vanilla data export completed: {} files written to {}", files.size(), output_path.string());
        }
        catch (const std::exception &error) {
            logger.error("Vanilla data export failed: {}", error.what());
        }
        gExportInProgress = false;
    });

    return true;
}

}  // namespace endstone::core
