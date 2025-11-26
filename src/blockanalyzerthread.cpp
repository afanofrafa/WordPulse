#include "blockanalyzerthread.h"

BlockAnalyzerThread::BlockAnalyzerThread(const Config &config, IDataProvider* dataProvider_ptr, QObject *parent)
    : QThread{parent}, config(config)
{
    this->dataProvider_ptr = dataProvider_ptr;
    word.reserve(50);

    totalSize = 0;
    processed = 0;

    update_timer = new QTimer(this);
    connect(update_timer, &QTimer::timeout, this, &BlockAnalyzerThread::emitUpdate, Qt::QueuedConnection);

    //connect(dataProvider_ptr, &FileReaderThread::chunkIsReady, this, &BlockAnalyzerThread::analyzeBlock);

    regex.setPattern(config.string_pattern);
    if (!config.case_sensitive) {
        regex.setPatternOptions(QRegularExpression::NoPatternOption);
    } else {
        regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }

    qDebug() << "BlockAnalyzerThread initialized.";
}

BlockAnalyzerThread::~BlockAnalyzerThread()
{
    qInfo() << "BlockAnalyzerThread destroying...";

    cancelAnalyzis();

    if (isRunning()) {
        requestInterruption(); // Вежливая просьба
        quit();
        if (!wait(3000)) {
            qCritical() << "BlockAnalyzerThread stuck, terminating.";
            terminate();
        }
    }
}

void BlockAnalyzerThread::analyzeBlock(void)
{
    if (!dataProvider_ptr) {
        qCritical() << "FileReader pointer is null in analyzeBlock";
        return;
    }

    QByteArrayView block;
    try
    {
        {
            dataProvider_ptr->lock();
            if (dataProvider_ptr->isDataEmpty()) {
                dataProvider_ptr->unlock();
                return;
            }

            block = dataProvider_ptr->getDataBlock();
            dataProvider_ptr->unlock();
        }
        // QByteArrayView view = block.right(15);
        // qDebug() << "right part of block: " << view;
        QRegularExpressionMatchIterator it = regex.globalMatch(QString::fromUtf8(block));

        while (it.hasNext())
        {
            QRegularExpressionMatch match = it.next();
            if (!match.hasMatch())
                continue;

            if (regex.patternOptions() == QRegularExpression::NoPatternOption) {
                word = match.captured(0).toLower();
            }
            else {
                word = match.captured(0);
            }
            if (word.isEmpty())
                continue;

            quint64 &count = totalWordsMap[word];
            quint64 oldCount = count;
            ++count;

            if (oldCount > 0) {
                topWordsSet.erase({oldCount, word});
            }

            topWordsSet.insert({count, word});

            while (topWordsSet.size() > static_cast<size_t>(config.top_n) )
            {
                topWordsSet.erase(topWordsSet.begin());
            }
        }

        processed += block.size();
    }
    catch (const std::exception &e) {
        qCritical() << "Exception during block analysis:" << e.what();
    }
}

QVector<QPair<quint64, QString>> BlockAnalyzerThread::getTopWordsWithCount(void) const
{
    QVector<QPair<quint64, QString>> result;
    const size_t n = qMin(static_cast<size_t>(config.top_n), topWordsSet.size());
    if (!n)
        return result;

    result.reserve(n);
    auto it = topWordsSet.rbegin();
    for (size_t i = 0; i < n; ++i, ++it) {
        result.prepend({it->first, it->second});
    }
    return result;
}

void BlockAnalyzerThread::setTotalSize(quint64 totalSize)
{
    this->totalSize = totalSize;
}

void BlockAnalyzerThread::setProcessed(quint64 processed)
{
    this->processed = processed;
}

void BlockAnalyzerThread::clearTops()
{
    topWordsSet.clear();
    totalWordsMap.clear();
}

void BlockAnalyzerThread::cancelAnalyzis(void)
{
    qInfo() << "Analysis canceled.";
    if (update_timer && update_timer->isActive())
        update_timer->stop();

    topWordsSet.clear();
    totalWordsMap.clear();
    processed = 0;

    QMetaObject::invokeMethod(this, &BlockAnalyzerThread::emitUpdate, Qt::QueuedConnection);
}

void BlockAnalyzerThread::startAnalyzis(void)
{
    qInfo() << "Analysis started.";
    topWordsSet.clear();
    totalWordsMap.clear();
    processed = 0;

    if (update_timer && !update_timer->isActive())
        update_timer->start(config.update_interval_ms);
}

void BlockAnalyzerThread::resumeAnalyzis(void)
{
    qInfo() << "Analysis resumed.";
    if (update_timer && !update_timer->isActive())
        update_timer->start(config.update_interval_ms);
}

void BlockAnalyzerThread::pauseAnalyzis(void)
{
    qInfo() << "Analysis paused.";
    if (update_timer && update_timer->isActive())
        update_timer->stop();
}

void BlockAnalyzerThread::run()
{
    exec();
}

void BlockAnalyzerThread::emitUpdate(void)
{
    qInfo() << "Emit update";
    if (totalSize == 0) {
        if (processed > 0)
            emit progress(0);
        return;
    }

    double ratio = static_cast<double>(processed) / static_cast<double>(totalSize);
    quint8 progressPercent = static_cast<quint8>(qBound(0.0, ratio * 100.0, 100.0));

    QVector<QPair<quint64, QString>> loc_topWords = getTopWordsWithCount();

    if (progressPercent == 100) {
        qInfo() << "Analysis reached 100%";
    }

    emit progress(progressPercent);
    emit topWords(loc_topWords);
}
