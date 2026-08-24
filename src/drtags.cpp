#include "drtags.h"

namespace Fooyin::DRMeter {
Track prepareTaggedTrack(const Track& track, int trackScore, std::optional<int> albumScore)
{
    Track tagged{track};
    tagged.replaceExtraTag(QString::fromLatin1(TrackTag), QString::number(trackScore));
    if(albumScore) {
        tagged.replaceExtraTag(QString::fromLatin1(AlbumTag), QString::number(*albumScore));
    }
    return tagged;
}
} // namespace Fooyin::DRMeter
