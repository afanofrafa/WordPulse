#ifndef WORDPULSEVIEWMODEL_H
#define WORDPULSEVIEWMODEL_H

#include <QObject>
#include <QUrl>
#include <QtQml>
#include <QVector>
#include <memory>
#include "filereaderthread.h"
#include "blockanalyzerthread.h"
//#include "topwordsmodel.h"
#include "config.h"
//class FileReaderThread;
class TopWordsModel;

class WordPulseViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(quint8 progress READ get_progress NOTIFY progressChanged)
    //Q_PROPERTY(QVariantList topWords READ get_topWords NOTIFY topWordsChanged)
    Q_PROPERTY(TopWordsModel* topWordsModel READ getTopWordsModel NOTIFY topWordsChanged)
    Q_PROPERTY(QString status READ get_status NOTIFY statusChanged)

    Q_PROPERTY(quint64 topWordsCount READ get_topWordsCount NOTIFY topWordsCountChanged)

    Q_PROPERTY(bool isRunning READ get_isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool isPaused READ get_isPaused NOTIFY pausedChanged)

public:
    explicit WordPulseViewModel(QObject* parent = nullptr);
    ~WordPulseViewModel();

    quint8 get_progress() const noexcept;
    //QVariantList get_topWords() const noexcept;
    TopWordsModel* getTopWordsModel() const noexcept;
    QString get_status() const noexcept;

    qint32 get_topWordsCount() const noexcept;
    bool get_isRunning() const noexcept;
    bool get_isPaused() const noexcept;

public slots:
    void openFile();
    void start();
    void pause();
    void resume();
    void cancel();

signals:
    void progressChanged();
    void topWordsChanged();
    void topWordsCountChanged();
    void statusChanged();

    void runningChanged();
    void pausedChanged();
    // Сигналы к FileReader
    void readingStarted();
    void readingPaused();
    void readingResumed();
    void readingCancel();
// signals:
//     void progressChanged(int);
//     void runningChanged();
//     void pausedChanged();

private:
    //void setProgress(int value);
    void setStatus(const QString& status_str);
    void setError(const QString& error_str);
    void setProgress(quint8 progress);
    //void setTopWords(const QVariantList& topWords);
    void setIsRunning(bool isRunning);
    void setIsPaused(bool isPaused);
    void updateProgress(quint8 progress);
    //void updateTopWords();
    void updateTopWords(const QVector<QPair<quint64, QString>>& newTopWords);

    std::unique_ptr<FileReaderThread> reader;
    std::unique_ptr<BlockAnalyzerThread> analyzer;

    TopWordsModel* _topWordsModel;

    QString _configPath;
    const Config _config;

    QString _status;
    QString _error;
    quint8 _progress;

    bool _isRunning;
    bool _isPaused;
};

#endif // WORDPULSEVIEWMODEL_H
