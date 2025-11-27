#include "wordpulseviewmodel.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include "topwordsmodel.h"

WordPulseViewModel::WordPulseViewModel(QObject *parent) : QObject{parent}, _configPath("config.json"), _config(Config::fromJson(_configPath))
{
    _topWordsModel = new TopWordsModel(this);
    //update_progress_timer = new QTimer(this);
    //connect(update_progress_timer, &QTimer::timeout, this, &WordPulseViewModel::updateProgress, Qt::QueuedConnection);
    //connect(analyzer.get(), &BlockAnalyzerThread::topWords, this, &WordPulseViewModel::updateTopWords);
    //connect(update_progress_timer, &QTimer::timeout, this, &WordPulseViewModel::updateTopWords, Qt::QueuedConnection);
    _progress = 0;
     _topWordsModel->resetTopWords({});//topWords.reserve(config.top_n);
    _status = "";
    _isRunning = false;
    _isPaused = false;

    reader = std::make_unique<FileReaderThread>("", _config);
    analyzer = std::make_unique<BlockAnalyzerThread>(_config, reader.get());
    connect(reader.get(), &FileReaderThread::chunkIsReady,
            analyzer.get(), &BlockAnalyzerThread::analyzeBlock, Qt::QueuedConnection);

    connect(this, &WordPulseViewModel::readingStarted, reader.get(), &FileReaderThread::startReading, Qt::QueuedConnection);
    connect(this, &WordPulseViewModel::readingStarted, analyzer.get(), &BlockAnalyzerThread::startAnalyzis, Qt::QueuedConnection);

    connect(this, &WordPulseViewModel::readingPaused, reader.get(), &FileReaderThread::pauseReading, Qt::QueuedConnection);
    connect(this, &WordPulseViewModel::readingPaused, analyzer.get(), &BlockAnalyzerThread::pauseAnalyzis, Qt::QueuedConnection);

    connect(this, &WordPulseViewModel::readingResumed, reader.get(), &FileReaderThread::resumeReading, Qt::QueuedConnection);
    connect(this, &WordPulseViewModel::readingResumed, analyzer.get(), &BlockAnalyzerThread::resumeAnalyzis, Qt::QueuedConnection);

    connect(this, &WordPulseViewModel::readingCancel, reader.get(), &FileReaderThread::cancelReading, Qt::QueuedConnection);
    connect(this, &WordPulseViewModel::readingCancel, analyzer.get(), &BlockAnalyzerThread::cancelAnalyzis, Qt::QueuedConnection);

    connect(reader.get(), &FileReaderThread::isPausedChanged, this, &WordPulseViewModel::setIsPaused, Qt::QueuedConnection);
    connect(reader.get(), &FileReaderThread::isRunningChanged, this, &WordPulseViewModel::setIsRunning, Qt::QueuedConnection);

    connect(analyzer.get(), &BlockAnalyzerThread::progress, this, &WordPulseViewModel::updateProgress, Qt::QueuedConnection);
    connect(analyzer.get(), &BlockAnalyzerThread::topWords, this, &WordPulseViewModel::updateTopWords,  Qt::QueuedConnection);
}

WordPulseViewModel::~WordPulseViewModel()
{
    /*if (update_progress_timer) {
        update_progress_timer->stop();
        update_progress_timer->deleteLater();
    }*/
}

QString WordPulseViewModel::get_status() const noexcept
{
    return _status;
}

qint32 WordPulseViewModel::get_topWordsCount() const noexcept
{
    return _config.top_n;
}

bool WordPulseViewModel::get_isRunning() const noexcept
{
    return _isRunning;
}

bool WordPulseViewModel::get_isPaused() const noexcept
{
    return _isPaused;
}

quint8 WordPulseViewModel::get_progress() const noexcept
{
    return _progress;
}

// QVariantList WordPulseViewModel::get_topWords() const noexcept
// {
//     return topWords;
// }

void WordPulseViewModel::openFile() {
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,                                   // родитель (nullptr = модальный)
        tr("Открыть файл"),              // заголовок
        QDir::homePath(),                          // начальная папка
        tr("Текстовые файлы (*.txt *.log);;Все файлы (*)")  // фильтры
    );

    if (fileName.isEmpty()) {
        setStatus("Выбор файла отменён");
        return;
    }

    reader->setFilePath(fileName);

    QFileInfo fileInfo(fileName);
    if (analyzer) {
         analyzer->setProcessed(0);
         analyzer->clearTops();
         //analyzer->setTop(0);
         analyzer->setTotalSize(fileInfo.size());
    }
//C:\Users\Archie\Documents\Work_C\untitled 2\untitled
     _topWordsModel->resetTopWords({});
    //topWords.clear();
    _progress = 0;
    //emit topWordsChanged();
    emit progressChanged();

    setStatus(QString("Выбран файл: %1 (%2 КБ)")
                  .arg(fileInfo.fileName())
                  .arg(fileInfo.size() / 1024));
}

void WordPulseViewModel::start()
{
    qDebug() << "started";
    _topWordsModel->resetTopWords({});
    //topWords.clear();
    _progress = 0;
    //emit topWordsChanged();
    emit progressChanged();

    // if (update_progress_timer && !update_progress_timer->isActive())
    //     update_progress_timer->start(config.update_interval_ms);

    if (reader)
        reader->start();
    if (analyzer)
        analyzer->start();

    emit readingStarted();
}

void WordPulseViewModel::pause()
{
    qDebug() << "paused";
    // if (update_progress_timer && update_progress_timer->isActive())
    //     update_progress_timer->stop();

    emit readingPaused();
}

void WordPulseViewModel::resume()
{
    // if (update_progress_timer && !update_progress_timer->isActive())
    //     update_progress_timer->start(config.update_interval_ms);

    qDebug() << "resumed";
    emit readingResumed();
}

void WordPulseViewModel::cancel()
{
    qDebug() << "cancel";
     _topWordsModel->resetTopWords({});
    //topWords.clear();
    _progress = 0;
    //emit topWordsChanged();
    emit progressChanged();

    // if (update_progress_timer && update_progress_timer->isActive())
    //     update_progress_timer->stop();

    emit readingCancel();
}

void WordPulseViewModel::setStatus(const QString& status_str) {
    if (_status != status_str) {
        _status = status_str;
        emit statusChanged();
    }
}

void WordPulseViewModel::setError(const QString& error_str)
{
    if (_error != error_str)
        _error = error_str;
}

void WordPulseViewModel::setProgress(quint8 progress)
{
    if (this->_progress != progress) {
        this->_progress = progress;
    }
}

// void WordPulseViewModel::setTopWords(const QVariantList &topWords)
// {
//     if  (this->topWords != topWords) {
//         this->topWords = topWords;
//         isTopWordsListChanged = true;
//     }

// }

void WordPulseViewModel::setIsRunning(bool isRunning)
{
    this->_isRunning = isRunning;
    qDebug() << "setIsRunning: " << isRunning;
    emit runningChanged();
}

void WordPulseViewModel::setIsPaused(bool isPaused)
{
    this->_isPaused = isPaused;
    qDebug() << "setIsPaused: " << isPaused;
    emit pausedChanged();
}

void WordPulseViewModel::updateProgress(quint8 progress) {
    if (this->_progress != progress) {
        this->_progress = progress;
        qDebug() << "progressChanged";
        emit progressChanged();
    }
}

void WordPulseViewModel::updateTopWords(const QVector<QPair<quint64, QString>>& newTopWords) {
    const int currentSize = _topWordsModel->rowCount();
    const int newSize = newTopWords.size();
    const int minSize = qMin(currentSize, newSize);

    bool needsReset = (currentSize != newSize);
    if (!needsReset) {
        for (int i = 0; i < minSize; ++i) {
            const auto& newPair = newTopWords.at(i);
            const QModelIndex idx = _topWordsModel->index(i);
            quint64 currentCount = _topWordsModel->data(idx, TopWordsModel::CountRole).value<quint64>();
            QString currentWord = _topWordsModel->data(idx, TopWordsModel::WordRole).toString();
            if (newPair.first != currentCount || newPair.second != currentWord) {
                _topWordsModel->updateWord(i, newPair.first, newPair.second);
            }
        }
    } else {
        // Редко: размер изменился (напр., с 0 до 15)
        _topWordsModel->resetTopWords(newTopWords);
    }
    //emit topWordsChanged();  // Не обязательно, dataChanged уже обновит QML
}

TopWordsModel* WordPulseViewModel::getTopWordsModel() const noexcept {
    return _topWordsModel;
}
// void WordPulseViewModel::updateTopWords() {
//     if (isTopWordsListChanged) {
//         emit topWordsChanged();
//         isTopWordsListChanged = false;
//     }
// }
