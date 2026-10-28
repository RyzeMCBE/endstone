// Copyright (c) 2026, The Endstone Project. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace endstone::core::devtools {

/**
 * Captures the unmodified, framed JigsawStructureDataPacket sent by BDS.
 *
 * The packet's network ID is 313 in Bedrock 1.26.60. This avoids interpreting
 * unverified BDS JigsawStructureRegistry memory layouts. A compatible client
 * must cause BDS to send this packet before an export is available.
 */
class JigsawPacketCapture {
public:
    static constexpr int PacketId = 313;

    static void capture(std::string_view framed_packet)
    {
        constexpr std::size_t MaxPacketBytes = 64U * 1024U * 1024U;
        if (framed_packet.empty() || framed_packet.size() > MaxPacketBytes) {
            return;
        }
        std::lock_guard lock(mutex());
        if (storedPacket().empty()) {
            storedPacket().assign(framed_packet);
        }
    }

    [[nodiscard]] static bool hasPacket()
    {
        std::lock_guard lock(mutex());
        return !storedPacket().empty();
    }

    [[nodiscard]] static std::optional<std::string> snapshot()
    {
        std::lock_guard lock(mutex());
        if (storedPacket().empty()) {
            return std::nullopt;
        }
        return storedPacket();
    }

private:
    [[nodiscard]] static std::mutex &mutex()
    {
        static std::mutex value;
        return value;
    }

    [[nodiscard]] static std::string &storedPacket()
    {
        static std::string value;
        return value;
    }
};

}  // namespace endstone::core::devtools
