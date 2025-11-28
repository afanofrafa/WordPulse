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
    Q_PROPERTY(TopWordsModel* topWordsModel READ getTopWordsModel NOTIFY topWordsChanged)

    Q_PROPERTY(quint64 topWordsCount READ get_topWordsCount NOTIFY topWordsCountChanged)

    Q_PROPERTY(bool isRunning READ get_isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool isPaused READ get_isPaused NOTIFY pausedChanged)

public:
    explicit WordPulseViewModel(QObject* parent = nullptr);
    ~WordPulseViewModel();

    quint8 get_progress() const noexcept;
    TopWordsModel* getTopWordsModel() const noexcept;

    qint32 get_topWordsCount() const noexcept;
    bool get_isRunning() const noexcept;
    bool get_isPaused() const noexcept;

    enum MessageType {
        MsgInfo = 0,
        MsgWarning,
        MsgError
    };
    Q_ENUM(MessageType)

public slots:
    void openFile();
    void start();
    void pause();
    void resume();
    void cancel();

signals:
    void systemMessage(WordPulseViewModel::MessageType type, const QString &text);

    void progressChanged();
    void topWordsChanged();
    void topWordsCountChanged();

    void runningChanged();
    void pausedChanged();

    void readingStarted();
    void readingPaused();
    void readingResumed();
    void readingCancel();

private:
    void showInfo(const QString &msg);
    void showWarning(const QString &msg);
    void showError(const QString &msg);

    void setProgress(quint8 progress);
    void setIsRunning(bool isRunning);
    void setIsPaused(bool isPaused);
    void updateProgress(quint8 progress);
    void updateTopWords(const QVector<QPair<quint64, QString>>& newTopWords);

    void finishProcess(void);

    std::unique_ptr<FileReaderThread> reader;
    std::unique_ptr<BlockAnalyzerThread> analyzer;

    TopWordsModel* _topWordsModel;

    QString _configPath;
    const Config _config;

    quint8 _progress;

    bool _isRunning;
    bool _isPaused;
    bool _fileChosen;
};

#endif // WORDPULSEVIEWMODEL_H
