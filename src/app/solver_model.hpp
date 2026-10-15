#pragma once

#include <QtQml/qqmlregistration.h>
#include <QObject>
#include <QString>
#include <QVariantList>

#include "wjs/dictionary.hpp"

namespace wjs::app {

// What the QML window talks to: it owns the dictionary, and turns a board the
// window has assembled into the list of words found on it. All of the solving
// is the library's; this is only the shape Qt Quick wants.
class SolverModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // One line saying what state the model is in ("42 words loaded", "no
    // dictionary at ...", "the board has no tiles").
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    // Whether solve() can be asked: a dictionary is loaded.
    Q_PROPERTY(bool ready READ ready NOTIFY statusChanged)
    // One map per word found: word, score and path ("r1c1 r1c2 ...").
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(int totalScore READ totalScore NOTIFY resultsChanged)

public:
    explicit SolverModel(QObject* parent = nullptr);

    // The dictionary named by --dictionary on the command line, else
    // dictionaries/en.txt beside the executable.
    Q_INVOKABLE void loadDefaultDictionary();
    Q_INVOKABLE bool loadDictionary(const QString& path);

    // `boardText` uses the library's board syntax: rows separated by '/',
    // tiles by spaces.
    Q_INVOKABLE void solve(const QString& boardText, int minLength = 3);

    [[nodiscard]] QString status() const { return status_; }
    [[nodiscard]] bool ready() const { return ready_; }
    [[nodiscard]] QVariantList results() const { return results_; }
    [[nodiscard]] int totalScore() const { return total_score_; }

signals:
    void statusChanged();
    void resultsChanged();

private:
    void setStatus(QString status);

    wjs::Dictionary dictionary_;
    QString status_;
    bool ready_ = false;
    QVariantList results_;
    int total_score_ = 0;
};

}  // namespace wjs::app
