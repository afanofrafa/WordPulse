#include "blockanalyzerthread.h"
#include <QTextBoundaryFinder>
#include <QDebug>

BlockAnalyzerThread::BlockAnalyzerThread(const Config &config, QByteArrayView chunk, int threadId, QObject *parent)
    : QThread(parent), _config(config), _chunk(chunk), _threadId(threadId)
{
}

BlockAnalyzerThread::~BlockAnalyzerThread() {
    if (isRunning()) {
        requestInterruption();
        setPaused(false);
        wait();
    }
}

void BlockAnalyzerThread::setPaused(bool paused) {
    QMutexLocker locker(&_pauseMutex);
    _isPaused = paused;
    if (!paused) {
        _pauseCondition.wakeAll();
    }
}

void BlockAnalyzerThread::run() {
    qInfo() << "Run ID:" << QThread::currentThreadId() << "| Thread Index:" << _threadId;

    // 1. ЛОКАЛЬНЫЙ БУФЕР НА QStringView (ZERO ALLOCATION)
    // QHash работает за O(1), что в разы быстрее древовидного QMap
    QHash<QStringView, quint64> localFastBuffer;

    QElapsedTimer timer;
    timer.start();

    try {
        // ЕДИНСТВЕННАЯ аллокация на весь чанк для работы QTextBoundaryFinder
        QString textChunk = QString::fromUtf8(_chunk.data(), _chunk.size());

        // 2. ГЛОБАЛЬНЫЙ toLower()
        // Мы понижаем регистр всего мегабайтного текста разом, а не по одному слову в цикле!
        if (!_config.case_sensitive) {
            textChunk = textChunk.toLower();
        }

        // Создаем View на весь наш чанк
        QStringView chunkView(textChunk);

        QTextBoundaryFinder finder(QTextBoundaryFinder::Word, textChunk);

        qsizetype startPos = finder.position();
        qsizetype totalLength = textChunk.length();
        qint8 lastReportedProgress = -1;

        while (finder.toNextBoundary() != -1) {
            if (isInterruptionRequested())
                return;

            // --- БЛОК ОЖИДАНИЯ ПАУЗЫ ---
            {
                QMutexLocker locker(&_pauseMutex);
                while (_isPaused) {
                    _pauseCondition.wait(&_pauseMutex);
                    if (isInterruptionRequested())
                        return;
                }
            }

            qsizetype endPos = finder.position();

            if (finder.boundaryReasons() & QTextBoundaryFinder::EndOfItem) {
                // 3. ZERO-ALLOCATION извлечение слова
                QStringView wordView = chunkView.mid(startPos, endPos - startPos);

                bool isActualWord = false;
                for (const QChar& c : wordView) {
                    if (c.isLetterOrNumber()) {
                        isActualWord = true;
                        break;
                    }
                }

                if (isActualWord) {
                    // 4. ZERO-ALLOCATION вставка в локальный буфер.
                    // Если слово уже есть в QHash, он просто увеличит счетчик.
                    localFastBuffer[wordView]++;
                }
            }
            startPos = endPos;

            // --- ОТПРАВКА ПОРЦИИ ДАННЫХ В REAL-TIME ---
            if (timer.elapsed() >= _config.update_interval_ms) {
                if (!localFastBuffer.isEmpty()) {

                    // 5. КОНВЕРТАЦИЯ В QString ТОЛЬКО ПЕРЕД ОТПРАВКОЙ (1 раз для каждого уникального слова)
                    QMap<QString, quint64> exportMap;
                    for (auto it = localFastBuffer.constBegin(); it != localFastBuffer.constEnd(); ++it) {
                        exportMap.insert(it.key().toString(), it.value());
                    }

                    emit intermediateData(exportMap);
                    localFastBuffer.clear(); // Очищаем локальный буфер
                }
                timer.restart();
            }

            // --- ОБНОВЛЕНИЕ ПРОГРЕССА ---
            quint8 currentProgress = static_cast<quint8>((static_cast<quint64>(endPos) * 100) / static_cast<quint64>(totalLength));
            if (currentProgress != lastReportedProgress && currentProgress % 2 == 0) {
                emit progressUpdated(_threadId, currentProgress);
                lastReportedProgress = currentProgress;
            }
        }

        // Отправляем остатки после завершения цикла
        if (!localFastBuffer.isEmpty()) {
            QMap<QString, quint64> exportMap;
            for (auto it = localFastBuffer.constBegin(); it != localFastBuffer.constEnd(); ++it) {
                exportMap.insert(it.key().toString(), it.value());
            }
            emit intermediateData(exportMap);
        }

        emit progressUpdated(_threadId, 100);
        emit analysisFinished();

    } catch (const std::exception &e) {
        emit analyzingError(QString("Exception in analyzer thread: ") + e.what());
    }
}
