#ifndef BLOCKANALYZERTHREAD_H
#define BLOCKANALYZERTHREAD_H

#include <QThread>
#include <QMap>
#include <QByteArrayView>
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include "config.h"

class BlockAnalyzerThread : public QThread {
    Q_OBJECT

public:
    explicit BlockAnalyzerThread(const Config& config,
                                 QByteArrayView chunk,
                                 int threadId,
                                 QObject* parent = nullptr);
    ~BlockAnalyzerThread() override;

    void setPaused(bool paused);

signals:
    void intermediateData(const QMap<QString, quint64>& chunkMap);
    void analysisFinished(/*const QMap<QString, quint64>& localWordMap*/);
    void progressUpdated(int threadId, quint8 progress);
    void analyzingError(const QString& error);

protected:
    void run() override;

private:
    const Config& _config;
    QByteArrayView _chunk;
    int _threadId;
    std::atomic<bool> _isPaused{false};
    QMutex _pauseMutex;
    QWaitCondition _pauseCondition;
};

#endif // BLOCKANALYZERTHREAD_H
