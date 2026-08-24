#include "drtags.h"

namespace Fooyin::DRMeter {
namespace {
constexpr QChar GroupSeparator{0x1f};
}

Track prepareTaggedTrack(const Track& track, int trackScore, std::optional<int> albumScore)
{
    Track tagged{track};
    tagged.replaceExtraTag(QString::fromLatin1(TrackTag), QString::number(trackScore));
    if(albumScore) {
        tagged.replaceExtraTag(QString::fromLatin1(AlbumTag), QString::number(*albumScore));
    }
    return tagged;
}

QString albumGroupKey(const Track& track)
{
    return track.albumArtist() + GroupSeparator + track.date() + GroupSeparator + track.album();
}

QString albumGroupLabel(const Track& track)
{
    return QStringLiteral("%1 - %2 - %3").arg(track.albumArtist(), track.date(), track.album());
}
} // namespace Fooyin::DRMeter
