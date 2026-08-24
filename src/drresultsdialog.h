#pragma once

#include "drscanner.h"

#include <QDialog>

#include <functional>
#include <memory>

class QDialogButtonBox;
class QLabel;
class QPushButton;
class QTableWidget;

namespace Fooyin {
class AudioLoader;
class MusicLibrary;
}

namespace Fooyin::DRMeter {
class DRResultsDialog : public QDialog
{
    Q_OBJECT

public:
    DRResultsDialog(MusicLibrary* library, std::shared_ptr<AudioLoader> audioLoader, TrackDRResults results,
                    ScanMode mode, QWidget* parent = nullptr);

protected:
    void reject() override;

private:
    [[nodiscard]] bool isWritable(const TrackDRResult& result) const;
    void writeTags();

    MusicLibrary* m_library;
    std::shared_ptr<AudioLoader> m_audioLoader;
    TrackDRResults m_results;
    ScanMode m_mode;
    int m_albumScore{0};
    bool m_writing{false};
    std::function<void()> m_cancelWrite;

    QTableWidget* m_table;
    QLabel* m_status;
    QDialogButtonBox* m_buttons;
    QPushButton* m_writeButton;
};
} // namespace Fooyin::DRMeter
