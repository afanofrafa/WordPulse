#ifndef WORDPULSEVIEWMODEL_H
#define WORDPULSEVIEWMODEL_H

#include <QObject>
#include <QtQml>
#include <QVector>
#include <QThread> // <-- Добавлен инклюд
#include "config.h"

class AnalyzerOrchestrator;
class TopWordsModel;

class WordPulseViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(quint8 progress READ get_progress NOTIFY progressChanged)
    Q_PROPERTY(TopWordsModel* topWordsModel READ getTopWordsModel NOTIFY topWordsChanged)
    Q_PROPERTY(bool isRunning READ get_isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool isPaused READ get_isPaused NOTIFY pausedChanged)

public:
    explicit WordPulseViewModel(QObject* parent = nullptr);
    ~WordPulseViewModel();

    quint8 get_progress() const noexcept { return _progress; }
    TopWordsModel* getTopWordsModel() const noexcept { return _topWordsModel; }
    bool get_isRunning() const noexcept { return _isRunning; }
    bool get_isPaused() const noexcept { return _isPaused; }

    enum MessageType { MsgInfo = 0, MsgWarning, MsgError };
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
    void runningChanged();
    void pausedChanged();

private slots:
    void onTopWordsReady(const QVector<QPair<quint64, QString>>& topWords);
    void onProgressUpdated(quint8 progress);
    void onAnalysisFinished();
    void onAnalysisError(const QString& error);
    void onAnalysisCanceled();

private:
    void showInfo(const QString &msg) { emit systemMessage(MsgInfo, msg); }
    void showWarning(const QString &msg) { emit systemMessage(MsgWarning, msg); }
    void showError(const QString &msg) { emit systemMessage(MsgError, msg); }
    void setProgress(quint8 progress);

    // Функция для жесткой синхронной очистки потока
    void cleanupOrchestrator();

    TopWordsModel* _topWordsModel;
    QString _configPath;
    const Config _config;
    QString _currentFilePath;

    AnalyzerOrchestrator* _orchestrator = nullptr;
    QThread* _orchestratorThread = nullptr; // Новый поток!

    quint8 _progress;
    bool _isRunning;
    bool _isPaused;
    bool _fileChosen;
};

#endif // WORDPULSEVIEWMODEL_H
