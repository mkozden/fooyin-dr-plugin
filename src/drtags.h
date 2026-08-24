#pragma once

#include <core/track.h>

#include <optional>

namespace Fooyin::DRMeter {
inline constexpr auto TrackTag = "DYNAMIC RANGE";
inline constexpr auto AlbumTag = "ALBUM DYNAMIC RANGE";

[[nodiscard]] Track prepareTaggedTrack(const Track& track, int trackScore, std::optional<int> albumScore);
[[nodiscard]] QString albumGroupKey(const Track& track);
[[nodiscard]] QString albumGroupLabel(const Track& track);
} // namespace Fooyin::DRMeter
