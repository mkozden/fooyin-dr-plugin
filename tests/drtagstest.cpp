#include "drtags.h"

#include <QtTest>

using namespace Fooyin::DRMeter;

class DRTagsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void trackScanPreservesAlbumTag()
    {
        Fooyin::Track track;
        track.replaceExtraTag(QString::fromLatin1(AlbumTag), QStringLiteral("12"));

        const Fooyin::Track tagged = prepareTaggedTrack(track, 9, std::nullopt);
        QCOMPARE(tagged.extraTag(QString::fromLatin1(TrackTag)), QStringList{QStringLiteral("9")});
        QCOMPARE(tagged.extraTag(QString::fromLatin1(AlbumTag)), QStringList{QStringLiteral("12")});
    }

    void albumScanReplacesBothTags()
    {
        Fooyin::Track track;
        track.replaceExtraTag(QString::fromLatin1(TrackTag), QStringLiteral("4"));
        track.replaceExtraTag(QString::fromLatin1(AlbumTag), QStringLiteral("5"));

        const Fooyin::Track tagged = prepareTaggedTrack(track, 10, 11);
        QCOMPARE(tagged.extraTag(QString::fromLatin1(TrackTag)), QStringList{QStringLiteral("10")});
        QCOMPARE(tagged.extraTag(QString::fromLatin1(AlbumTag)), QStringList{QStringLiteral("11")});
    }

    void albumGroupingUsesAlbumArtistDateAndAlbum()
    {
        Fooyin::Track first;
        first.setAlbumArtists({QStringLiteral("Artist")});
        first.setDate(QStringLiteral("2020"));
        first.setAlbum(QStringLiteral("Album"));

        Fooyin::Track sameAlbum{first};
        sameAlbum.setTitle(QStringLiteral("Another track"));
        QCOMPARE(albumGroupKey(first), albumGroupKey(sameAlbum));
        QCOMPARE(albumGroupLabel(first), QStringLiteral("Artist - 2020 - Album"));

        Fooyin::Track reissue{first};
        reissue.setDate(QStringLiteral("2021"));
        QVERIFY(albumGroupKey(first) != albumGroupKey(reissue));
    }
};

QTEST_GUILESS_MAIN(DRTagsTest)

#include "drtagstest.moc"
