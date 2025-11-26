#ifndef BLOCKANALYZERTHREAD_H
#define BLOCKANALYZERTHREAD_H

#include <QObject>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QTimer>
#include <QRegularExpression>
#include <QMap>
#include "config.h"
#include "filereaderthread.h"
/*#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/concurrent_queue.h>
#include <oneapi/tbb/concurrent_map.h>
#pragma pop_macro("emit")*/

class BlockAnalyzerThread : public QThread {
    Q_OBJECT

public:
    explicit BlockAnalyzerThread(const Config& config,
                                 IDataProvider* dataProvider_ptr = nullptr,
                                 QObject* parent = nullptr);
    ~BlockAnalyzerThread() override;

public slots:
    void analyzeBlock(void);
    void setTotalSize(quint64 totalSize);
    void setProcessed(quint64 processed);
    void clearTops();
    void cancelAnalyzis(void);
    void startAnalyzis(void);
    void resumeAnalyzis(void);
    void pauseAnalyzis(void);

signals:
    void blockProcessed(const QMap<QByteArray, int>& wordCount, qint64 bytesProcessed);
    void error(const QString& message);
    void progress(quint8 progress);
    void topWords(const QVector<QPair<quint64, QString>>& list);

protected:
    void run() override;
private:
    void emitUpdate(void);
    QVector<QPair<quint64, QString>> getTopWordsWithCount(void) const;

    QByteArrayView block;
    QString word;

    const Config& config;
    QRegularExpression regex;
    QMap<QString, quint64> totalWordsMap;
    std::set<QPair<quint64, QString>, std::less<QPair<quint64, QString>>> topWordsSet;
    IDataProvider* dataProvider_ptr;
    quint64 totalSize;
    quint64 processed;
    QTimer* update_timer;
};

#endif // BLOCKANALYZERTHREAD_H
