#ifndef ANALYZERORCHESTRATOR_H
#define ANALYZERORCHESTRATOR_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QFile>
#include <QTimer>
#include <atomic>
#include "blockanalyzerthread.h"
#include "config.h"

// Наследуем от QObject, а не от QThread!
class AnalyzerOrchestrator : public QObject {
    Q_OBJECT
public:
    explicit AnalyzerOrchestrator(const Config& config, const QString& filePath, QObject* parent = nullptr);
    ~AnalyzerOrchestrator() override;

    void pause();
    void resume();

    // Атомарный флаг для быстрой остановки
    void requestCancel() { _isCanceled = true; }

signals:
    void sigTopWordsReady(const QVector<QPair<quint64, QString>>& topWords);
    void sigProgressUpdated(quint8 progress);
    void sigFinished();
    void sigError(const QString& error);
    void sigCanceled();

public slots:
    void startProcessing();
    void performCleanup();
    void cancelProcessing();

private slots:
    void onWorkerData(const QMap<QString, quint64>& chunkMap);
    void onWorkerProgress(int threadId, quint8 progress);
    void onWorkerError(const QString& error);
    void onWorkerFinished();
    void updateTopWords();

private:
    const Config _config;
    QString _filePath;
    QFile _file;
    uchar* _mappedData = nullptr;

    QMap<QString, quint64> _globalWordMap;
    QList<BlockAnalyzerThread*> _workers;
    QMap<int, quint8> _workersProgress;

    int _finishedWorkers = 0;
    std::atomic<bool> _isCanceled{false};

    QTimer* _updateTimer = nullptr;
};

#endif // ANALYZERORCHESTRATOR_H
