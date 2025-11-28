#ifndef FILEREADERTHREAD_H
#define FILEREADERTHREAD_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QQueue>
#include <QTimer>
#include <QFile>
#include "config.h"
#include "idataprovider.h"

//#pragma push_macro("emit")
//#undef emit
//#include <oneapi/tbb/concurrent_queue.h>
//#pragma pop_macro("emit")


class FileReaderThread : public QThread, public  IDataProvider
{
    Q_OBJECT
public:
    explicit FileReaderThread(const QString &filepath, const Config& config, QObject *parent = nullptr);

    ~FileReaderThread();

    void setFilePath(const QString &filepath);
    const QString& getFilePath(void) const noexcept;
    void triggerRead();

    void lock() override;
    void unlock() override;
    bool isDataEmpty() const noexcept override;
    qsizetype dataSize() const noexcept override;
    QByteArrayView getDataBlock() override;

    const QQueue<QByteArrayView>& getBlockQueue(void) const noexcept;
    bool getRunning() const noexcept;
    bool getPaused() const noexcept;

public slots:
    void startReading();
    void pauseReading();
    void resumeReading();
    void cancelReading();
    void readChunk();

signals:
    void chunkIsReady();
    void readingFinished(void);
    void readingError(const QString& error);
    void statusChanged(const QString &status_str);
    void error(const QString &error_str);
    void isRunningChanged(bool isRunning);
    void isPausedChanged(bool isPaused);

protected:
    void run() override;

private:
    QFile file;
    QString filePath;
    std::unique_ptr<QTextStream> stream;

    QQueue<QByteArrayView> blockQueue;
    QMutex mutex;

    const Config& config_cref;

    bool running;
    bool paused;

    friend class BlockAnalyzerThread;
};

#endif // FILEREADERTHREAD_H
