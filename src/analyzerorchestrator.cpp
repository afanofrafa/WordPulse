#include "analyzerorchestrator.h"
#include <algorithm>
#include <vector>

AnalyzerOrchestrator::AnalyzerOrchestrator(const Config& config, const QString& filePath, QObject* parent)
    : QObject(parent), _config(config), _filePath(filePath)
{
    qRegisterMetaType<QMap<QString, quint64>>("QMap<QString, quint64>");
    qRegisterMetaType<QVector<QPair<quint64, QString>>>("QVector<QPair<quint64, QString>>");
}

AnalyzerOrchestrator::~AnalyzerOrchestrator() {
    // Пустой, так как ViewModel вызывает performCleanup()
}

void AnalyzerOrchestrator::startProcessing() {
    _file.setFileName(_filePath);
    if (!_file.open(QIODevice::ReadOnly)) {
        emit sigError("Не удалось открыть файл: " + _file.errorString());
        return;
    }

    qint64 fileSize = _file.size();
    if (fileSize == 0) {
        emit sigError("Файл пуст!");
        return;
    }

    _mappedData = _file.map(0, fileSize);
    if (!_mappedData) {
        emit sigError("Не удалось смаппить файл в память.");
        return;
    }

    auto isUtf8ContinuationByte = [](uchar byte) { return (byte & 0xC0) == 0x80; };
    int numThreads = _config.num_threads;
    qint64 approximateChunkSize = fileSize / numThreads;
    qint64 currentOffset = 0;

    for (int i = 0; i < numThreads && currentOffset < fileSize; ++i) {
        qint64 endOffset = currentOffset + approximateChunkSize;

        if (i == numThreads - 1) {
            endOffset = fileSize;
        } else {
            int maxLookahead = 1024 * 1024 * 10;
            while (endOffset < fileSize && maxLookahead > 0) {
                uint8_t c = _mappedData[endOffset];
                // Используем таблицу из конфига!
                if (_config.is_delim_table[c]) break;
                endOffset++;
                maxLookahead--;

                while (endOffset < fileSize && isUtf8ContinuationByte(_mappedData[endOffset])) {
                    endOffset++;
                }
            }
        }

        qint64 chunkSize = endOffset - currentOffset;
        if (chunkSize > 0) {
            QByteArrayView chunk(reinterpret_cast<const char*>(_mappedData + currentOffset), chunkSize);

            // Родитель - this, так как мы уже находимся в нужном потоке
            auto* worker = new BlockAnalyzerThread(_config, chunk, i, this);
            connect(worker, &BlockAnalyzerThread::intermediateData, this, &AnalyzerOrchestrator::onWorkerData, Qt::QueuedConnection);
            connect(worker, &BlockAnalyzerThread::progressUpdated, this, &AnalyzerOrchestrator::onWorkerProgress, Qt::QueuedConnection);
            connect(worker, &BlockAnalyzerThread::analyzingError, this, &AnalyzerOrchestrator::onWorkerError, Qt::QueuedConnection);
            connect(worker, &BlockAnalyzerThread::analysisFinished, this, &AnalyzerOrchestrator::onWorkerFinished, Qt::QueuedConnection);

            _workersProgress[i] = 0;
            _workers.append(worker);
            worker->start(QThread::LowPriority);
        }
        currentOffset = endOffset;
    }

    _updateTimer = new QTimer(this);
    connect(_updateTimer, &QTimer::timeout, this, &AnalyzerOrchestrator::updateTopWords);
    _updateTimer->start(_config.update_interval_ms);
}

void AnalyzerOrchestrator::onWorkerData(const QMap<QString, quint64>& chunkMap) {
    if (_isCanceled) return;
    for (auto it = chunkMap.cbegin(); it != chunkMap.cend(); ++it) {
        _globalWordMap[it.key()] += it.value();
    }
}

void AnalyzerOrchestrator::updateTopWords() {
    if (_globalWordMap.isEmpty() || _isCanceled) return;

    std::vector<std::pair<quint64, QString>> vec;
    vec.reserve(_globalWordMap.size());
    for (auto it = _globalWordMap.cbegin(); it != _globalWordMap.cend(); ++it) {
        vec.emplace_back(it.value(), it.key());
    }

    size_t topN = std::min(static_cast<size_t>(_config.top_n), vec.size());

    std::partial_sort(vec.begin(), vec.begin() + topN, vec.end(),
                      [](const auto& a, const auto& b) { return a.first > b.first; });

    QVector<QPair<quint64, QString>> topWords;
    for (size_t i = 0; i < topN; ++i) {
        topWords.append({vec[i].first, vec[i].second});
    }

    emit sigTopWordsReady(topWords);
}

void AnalyzerOrchestrator::onWorkerProgress(int threadId, quint8 progress) {
    if (_isCanceled) return;
    _workersProgress[threadId] = progress;
    qsizetype totalProgress = 0;
    for (quint8 p : std::as_const(_workersProgress)) {
        totalProgress += p;
    }
    emit sigProgressUpdated(static_cast<quint8>(totalProgress / _workers.size()));
}

void AnalyzerOrchestrator::onWorkerFinished() {
    _finishedWorkers++;
    if (_finishedWorkers == _workers.size() && !_isCanceled) {
        if (_updateTimer) _updateTimer->stop();
        updateTopWords();
        emit sigProgressUpdated(100);
        emit sigFinished(); // Говорим ViewModel, что можно завершать
    }
}

void AnalyzerOrchestrator::onWorkerError(const QString& error) {
    emit sigError(error);
}

void AnalyzerOrchestrator::pause() {
    for (auto* worker : std::as_const(_workers)) if(worker) worker->setPaused(true);
}

void AnalyzerOrchestrator::resume() {
    for (auto* worker : std::as_const(_workers)) if(worker) worker->setPaused(false);
}

void AnalyzerOrchestrator::performCleanup() {
    if (_updateTimer) {
        _updateTimer->stop();
        delete _updateTimer;
        _updateTimer = nullptr;
    }

    for (auto* worker : std::as_const(_workers)) {
        if (worker) {
            worker->requestInterruption();
            worker->setPaused(false);
            worker->wait(); // Ждем работяг безопасно в фоне
            delete worker;
        }
    }
    _workers.clear();

    if (_mappedData && _file.isOpen()) {
        _file.unmap(_mappedData);
        _mappedData = nullptr;
    }
    if (_file.isOpen()) _file.close();
}

void AnalyzerOrchestrator::cancelProcessing() {
    if (_isCanceled)
        return;
    _isCanceled = true;

    performCleanup();

    emit sigCanceled();
}
