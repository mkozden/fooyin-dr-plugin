#pragma once

#include <core/track.h>

#include <optional>

namespace Fooyin::DRMeter {
inline constexpr auto TrackTag = "DYNAMIC RANGE";
inline constexpr auto AlbumTag = "ALBUM DYNAMIC RANGE";

[[nodiscard]] Track prepareTaggedTrack(const Track& track, int trackScore, std::optional<int> albumScore);
} // namespace Fooyin::DRMeter
