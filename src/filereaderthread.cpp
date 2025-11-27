#include "filereaderthread.h"
#include <QFileDialog>
#include <QTimer>
#include <QFileInfo>
#include <QDir>

FileReaderThread::FileReaderThread(const QString &filePath, const Config& config, QObject *parent) : QThread{parent}, config_cref(config)
{
    stream = std::make_unique<QTextStream>(&file);
    file.setFileName(filePath);
    this->filePath = filePath;

    running = false;
    paused = false;

    /*try {
        blockStr.reserve(config.chunk_size_bytes);
    } catch (const std::bad_alloc&) {
        qCritical() << "Failed to allocate memory for block buffer in FileReaderThread constructor";
    }*/
}

FileReaderThread::~FileReaderThread()
{
    qInfo() << "FileReaderThread is being destroyed";
    if (isRunning()) {
        qInfo() << "Stopping FileReaderThread...";
        quit();
        if (!wait(3000)) {
            qCritical() << "FileReaderThread did not stop gracefully, terminating.";
            terminate();
        } else {
            qInfo() << "FileReaderThread stopped successfully.";
        }
    }
}

void FileReaderThread::setFilePath(const QString &filepath)
{
    this->filePath = filepath;
    file.setFileName(filePath);
}

const QString& FileReaderThread::getFilePath(void) const noexcept
{
    return filePath;
}

void FileReaderThread::triggerRead()
{
    QMetaObject::invokeMethod(this, &FileReaderThread::readChunk, Qt::QueuedConnection);
}

void FileReaderThread::lock()
{
    mutex.lock();
}

void FileReaderThread::unlock()
{
    mutex.unlock();
}

bool FileReaderThread::isDataEmpty() const
{
    return blockQueue.isEmpty();
}

QByteArrayView FileReaderThread::getDataBlock()
{
    return blockQueue.dequeue();
}

QQueue<QByteArrayView>& FileReaderThread::getBlockQueue(void) noexcept
{
    return blockQueue;
}

void FileReaderThread::startReading() {
    if (running && file.isOpen()) {
        qWarning() << "Attempt to start reading while already running.";
        return;
    }

    if (running && paused) {
        qInfo() << "Resume reading requested via startReading.";
        resumeReading();
        return;
    }

    if (running) {
        cancelReading();
    }

    if (file.isOpen()) {
        file.close();
    }

    qInfo() << "Opening file for reading:" << filePath;
    if (!file.open(QIODevice::ReadOnly/* | QIODevice::Text*/)) {
        QString err = "Failed to open file: " + file.errorString();
        qCritical() << err;
        emit error(err);
        running = false;
        emit isRunningChanged(running);
        {
            QMutexLocker locker(&mutex);
            blockQueue.clear();
        }
        return;
    }

    running = true;
    paused = false;
    emit isRunningChanged(running);
    emit isPausedChanged(paused);

    {
        QMutexLocker locker(&mutex);
        blockQueue.clear();
    }

    emit statusChanged("Чтение начато: " + QFileInfo(filePath).fileName());

    qInfo() << "Reading started successfully.";

    triggerRead();
}

void FileReaderThread::pauseReading() {
    qInfo() << "Pause reading operation.";
    if (!running || paused)
        return;
    paused = true;
    emit isPausedChanged(paused);
    emit statusChanged("Пауза");
}

void FileReaderThread::resumeReading() {
    qInfo() << "Resume reading operation.";
    if (!running || !paused)
        return;
    paused = false;
    emit isPausedChanged(paused);
    emit statusChanged("Продолжение");

    triggerRead();
}

void FileReaderThread::cancelReading() {
    qInfo() << "Canceling reading operation.";
    running = false;
    paused = false;
    emit isRunningChanged(running);
    emit isPausedChanged(paused);

    {
        QMutexLocker locker(&mutex);
        blockQueue.clear();
    }

    if (file.isOpen()) {
        file.close();
    }

    emit statusChanged("Отменено");
}

/*void FileReaderThread::readChunk() {
    if (!running || paused)
        return;

    {
        QMutexLocker locker(&mutex);
        if (blockQueue.size() >= static_cast<quint64>(config_cref.max_chunks_in_mem_num))
            return;
    }

    if (file.atEnd()) {
        running = false;
        emit isRunningChanged(running);
        read_timer->stop();
        return;
    }

    bool isEnd = false;
    qsizetype cutPos;
    blockStr.resize(0);
    while (!isEnd) {
        if (file.size() == blockStr.size() || file.atEnd())
            break;

        blockStr += file.read(config_cref.chunk_size_bytes);

        // if (file.size() == blockStr.size())
        //     break;

        isEnd = true;
        for (cutPos = blockStr.size() - 1; isEnd; --cutPos) {
            char c = blockStr.at(cutPos);
            if (config_cref.word_separators.contains(c)) {
                break;
            }
            if (cutPos == 0)
                isEnd = false;
        }

        if (isEnd) {
            break;
        }
    }
    if (isEnd) {
        qint64 targetSize = blockStr.size();
        blockStr.resize(cutPos + 1);

        qint64 rollback = static_cast<qint64>((targetSize - 1) - cutPos);
        if (rollback)
            file.seek(file.pos() - rollback);
    }

    if (!blockStr.isEmpty())
    {
        QMutexLocker locker(&mutex);
        blockQueue.enqueue(blockStr);
    }
}*/
void FileReaderThread::readChunk() {
    if (!running || paused) {
         qDebug() << "Attempt to read while not running or pause. paused: " << paused << "; running: " << running << ".";
        return;
    }

    try {
        {
            QMutexLocker locker(&mutex);
            if (blockQueue.size() >= static_cast<int>(config_cref.max_chunks_in_mem_num)) {
                qDebug() << "Block queue full, yielding.";
                QThread::yieldCurrentThread();
                triggerRead();
                return;
            }
        }

        if (file.atEnd()) {
            running = false;
            qInfo() << "File reading completed (EOF reached).";
            emit isRunningChanged(running);
            return;
        }
        bool isEnd = false;
        QByteArrayView currentBlockView;
        qint64 currentPos = file.pos();
        qsizetype cutPos = currentPos;
        qint64 chunkSize = 0;
        while (!isEnd) {
            if (file.size() == currentBlockView.size() || file.atEnd() || file.size() <= chunkSize)
                break;

            chunkSize += qMin(config_cref.chunk_size_bytes, file.size() - currentPos);
            chunkSize = qMin(chunkSize, file.size());
            uchar* mapped = file.map(currentPos, chunkSize);
            if (!mapped) {
                running = false;
                QString err = "Critical: File mapping failed. Possibly out of memory or OS limit.";
                qCritical() << err;
                emit error(err);
                emit isRunningChanged(running);
                return;
            }

            currentBlockView = QByteArray::fromRawData(reinterpret_cast<const char*>(mapped), chunkSize);

            file.seek(currentPos + chunkSize);

            isEnd = true;
            for (cutPos = currentBlockView.size() - 1; isEnd/* && cutPos >= 0*/; --cutPos) {
                char c = currentBlockView.at(cutPos);
                if (config_cref.word_separators.contains(c)) {
                    break;
                }
                if (cutPos == 0)
                    isEnd = false;
            }

            if (isEnd) {
                break;
            }
        }
        if (isEnd) {
            qint64 targetSize = currentBlockView.size();
            currentBlockView = currentBlockView.first(cutPos + 1);

            qint64 rollback = static_cast<qint64>((targetSize - 1) - cutPos);
            //if (rollback)
            file.seek(currentPos + chunkSize - rollback);
        }

        if (!currentBlockView.isEmpty())
        {
            QMutexLocker locker(&mutex);
            blockQueue.enqueue(currentBlockView);
        }

        emit chunkIsReady();
        triggerRead();
    }
     catch (const std::exception &e) {
        qCritical() << "Exception in readChunk:" << e.what();
        running = false;
        emit error("Internal reading error");
        emit isRunningChanged(running);
    } catch (...) {
        qCritical() << "Unknown exception in readChunk";
        running = false;
        emit isRunningChanged(running);
    }
}

bool FileReaderThread::getRunning() const noexcept
{
    return running;
}

bool FileReaderThread::getPaused()  const noexcept
{
    return paused;
}
void FileReaderThread::run()
{
    exec();
}
