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
    if (!_dataProvider_ptr) {
        qCritical() << "FileReader pointer is null in analyzeBlock";
        return;
    }

    QByteArrayView block;
    try
    {
        {
            _dataProvider_ptr->lock();
            if (_dataProvider_ptr->isDataEmpty()) {
                _dataProvider_ptr->unlock();
                return;
            }

            block = _dataProvider_ptr->getDataBlock();
            _dataProvider_ptr->unlock();
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
    catch (const std::exception &e) {
        qCritical() << "Exception during block analysis:" << e.what();
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
    //qInfo() << "Emit update";
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
