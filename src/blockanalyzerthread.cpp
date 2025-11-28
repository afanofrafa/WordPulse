#include "blockanalyzerthread.h"

BlockAnalyzerThread::BlockAnalyzerThread(const Config &config, IDataProvider* dataProvider_ptr, QObject *parent)
    : QThread{parent}, _config(config)
{
    this->_dataProvider_ptr = dataProvider_ptr;
    _word.reserve(50);

    _totalSize = 0;
    _processed = 0;

    _update_timer = new QTimer(this);
    connect(_update_timer, &QTimer::timeout, this, &BlockAnalyzerThread::emitUpdate, Qt::QueuedConnection);

    //connect(dataProvider_ptr, &FileReaderThread::chunkIsReady, this, &BlockAnalyzerThread::analyzeBlock);

    _regex.setPattern(config.string_pattern);
    if (!_config.case_sensitive) {
        _regex.setPatternOptions(QRegularExpression::NoPatternOption);
    } else {
        _regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }

    this->moveToThread(this);

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

void BlockAnalyzerThread::analyzingFinishing(void)
{
    if (!_dataProvider_ptr)
        return;

    while (true) {
        _dataProvider_ptr->lock();
        bool b = !_dataProvider_ptr->isDataEmpty();
        _dataProvider_ptr->unlock();
        if (b)
            analyzeBlock();
        else
            break;
        //qDebug() << "iteration";
    }

    emit analyzisFinished();
}

void BlockAnalyzerThread::analyzeBlock(void)
{
    if (!_dataProvider_ptr) {
        qCritical() << "FileReader pointer is null in analyzeBlock";
        return;
    }

    QByteArrayView block;
    try
    {
        //QThread::sleep(std::chrono::seconds(1));
        {
            _dataProvider_ptr->lock();
            qsizetype size = _dataProvider_ptr->dataSize();
            if (_dataProvider_ptr->isDataEmpty()) {
                _dataProvider_ptr->unlock();
                return;
            }

            block = _dataProvider_ptr->getDataBlock();
            //qDebug() << "blockQueue dequeue";
            _dataProvider_ptr->unlock();
            if (size >= _config.max_chunks_in_mem_num
                && _dataProvider_ptr->dataSize() < _config.max_chunks_in_mem_num) {
                qInfo() << "threshold block freed";
                emit thresholdBlockFreed();
            }
        }
        // QByteArrayView view = block.right(15);
        // qDebug() << "right part of block: " << view;
        QRegularExpressionMatchIterator it = _regex.globalMatch(QString::fromUtf8(block));

        while (it.hasNext())
        {
            QRegularExpressionMatch match = it.next();
            if (!match.hasMatch())
                continue;

            if (_regex.patternOptions() == QRegularExpression::NoPatternOption) {
                _word = match.captured(0).toLower();
            }
            else {
                _word = match.captured(0);
            }
            if (_word.isEmpty())
                continue;

            quint64 &count = _totalWordsMap[_word];
            quint64 oldCount = count;
            ++count;

            if (oldCount > 0) {
                _topWordsSet.erase({oldCount, _word});
            }

            _topWordsSet.insert({count, _word});

            while (_topWordsSet.size() > static_cast<size_t>(_config.top_n) )
            {
                _topWordsSet.erase(_topWordsSet.begin());
            }
        }

        _processed += block.size();
    }
    catch (const std::bad_alloc &e) {
         qCritical() << "bad alloc exception:" << e.what();
         emit analyzingError(QString("bad alloc exception:") + e.what());
    }
    catch (const std::exception &e) {
        qCritical() << "Exception during block analysis:" << e.what();
        emit analyzingError(QString("Exception during block analysis:") + e.what());
    }
    catch (...) {
        qCritical() << "Unknown exception";
        emit analyzingError("Unknown exception");
    }
}

QVector<QPair<quint64, QString>> BlockAnalyzerThread::getTopWordsWithCount(void) const
{
    QVector<QPair<quint64, QString>> result;
    const size_t n = qMin(static_cast<size_t>(_config.top_n), _topWordsSet.size());
    if (!n)
        return result;

    result.reserve(n);
    auto it = _topWordsSet.rbegin();
    for (size_t i = 0; i < n; ++i, ++it) {
        result.prepend({it->first, it->second});
    }
    return result;
}

void BlockAnalyzerThread::setTotalSize(quint64 totalSize)
{
    this->_totalSize = totalSize;
}

void BlockAnalyzerThread::setProcessed(quint64 processed)
{
    this->_processed = processed;
}

void BlockAnalyzerThread::clearTops()
{
    _topWordsSet.clear();
    _totalWordsMap.clear();
}

void BlockAnalyzerThread::cancelAnalyzis(void)
{
    qInfo() << "Analysis canceled.";
    if (_update_timer && _update_timer->isActive())
        _update_timer->stop();

    _topWordsSet.clear();
    _totalWordsMap.clear();
    _processed = 0;

    QMetaObject::invokeMethod(this, &BlockAnalyzerThread::emitUpdate, Qt::QueuedConnection);
}

void BlockAnalyzerThread::startAnalyzis(void)
{
    qInfo() << "Analysis started.";
    _topWordsSet.clear();
    _totalWordsMap.clear();
    _processed = 0;

    if (_update_timer && !_update_timer->isActive())
        _update_timer->start(_config.update_interval_ms);
}

void BlockAnalyzerThread::resumeAnalyzis(void)
{
    qInfo() << "Analysis resumed.";
    if (_update_timer && !_update_timer->isActive())
        _update_timer->start(_config.update_interval_ms);
}

void BlockAnalyzerThread::pauseAnalyzis(void)
{
    qInfo() << "Analysis paused.";
    if (_update_timer && _update_timer->isActive())
        _update_timer->stop();
}

void BlockAnalyzerThread::run()
{
    exec();
}

void BlockAnalyzerThread::emitUpdate(void)
{
    if (_totalSize == 0) {
        if (_processed > 0)
            emit progress(0);
        return;
    }

    double ratio = static_cast<double>(_processed) / static_cast<double>(_totalSize);
    quint8 progressPercent = static_cast<quint8>(qBound(0.0, ratio * 100.0, 100.0));

    QVector<QPair<quint64, QString>> loc_topWords = getTopWordsWithCount();

    if (progressPercent == 100) {
        qInfo() << "Analysis reached 100%";
    }

    emit progress(progressPercent);
    emit topWords(loc_topWords);
}
