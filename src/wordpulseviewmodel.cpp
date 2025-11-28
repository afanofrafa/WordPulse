#include "wordpulseviewmodel.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include "topwordsmodel.h"

WordPulseViewModel::WordPulseViewModel(QObject *parent) : QObject{parent}, _configPath("config.json"), _config(Config::fromJson(_configPath))
{
    _topWordsModel = new TopWordsModel(this);
    _progress = 0;
     _topWordsModel->resetTopWords({});
    _status = "";
    _isRunning = false;
    _isPaused = false;
    _fileChosen = false;

    reader = std::make_unique<FileReaderThread>("", _config);
    analyzer = std::make_unique<BlockAnalyzerThread>(_config, reader.get());

    connect(reader.get(), &FileReaderThread::chunkIsReady,
            analyzer.get(), &BlockAnalyzerThread::analyzeBlock, Qt::QueuedConnection);

    connect(reader.get(), &FileReaderThread::readingFinished,
            analyzer.get(), &BlockAnalyzerThread::analyzingFinishing, Qt::QueuedConnection);

    connect(analyzer.get(), &BlockAnalyzerThread::analyzisFinished,
            this, &WordPulseViewModel::finishProcess, Qt::QueuedConnection);

    connect(analyzer.get(), &BlockAnalyzerThread::analyzingError,
            this, &WordPulseViewModel::showError, Qt::QueuedConnection);

    connect(reader.get(), &FileReaderThread::readingError,
            this, &WordPulseViewModel::showError, Qt::QueuedConnection);

    connect(analyzer.get(), &BlockAnalyzerThread::thresholdBlockFreed,
            reader.get(), &FileReaderThread::readChunk, Qt::QueuedConnection);

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

void WordPulseViewModel::openFile() {
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,                                   // родитель (nullptr = модальный)
        tr("Открыть файл"),              // заголовок
        QDir::homePath(),                          // начальная папка
        tr("Текстовые файлы (*.txt *.log);;Все файлы (*)")  // фильтры
    );

    if (fileName.isEmpty()) {
        emit showWarning("Выбор файла отменён");
        return;
    }

    reader->setFilePath(fileName);

    QFileInfo fileInfo(fileName);
    if (analyzer) {
         analyzer->setProcessed(0);
         analyzer->clearTops();
         analyzer->setTotalSize(fileInfo.size());
    }

    _topWordsModel->resetTopWords({});
    _progress = 0;
    _fileChosen = true;
    emit progressChanged();

    emit showInfo(QString("Выбран файл: %1 (%2 КБ)")
                  .arg(fileInfo.fileName())
                  .arg(fileInfo.size() / 1024));
}

void WordPulseViewModel::start()
{
    if (!_fileChosen) {
        emit showWarning("Файл не выбран!");
        return;
    }

    qDebug() << "started";
    _topWordsModel->resetTopWords({});
    _progress = 0;
    emit progressChanged();

    if (reader)
        reader->start();
    if (analyzer)
        analyzer->start();

    _isPaused = false;
    _isRunning = true;
    emit readingStarted();

    emit pausedChanged();
    emit runningChanged();
}

void WordPulseViewModel::pause()
{
    qDebug() << "paused";
    _isPaused = true;
    emit readingPaused();

    emit pausedChanged();
}

void WordPulseViewModel::resume()
{
    qDebug() << "resumed";
    _isPaused = false;
    emit readingResumed();

    emit pausedChanged();
}

void WordPulseViewModel::cancel()
{
    qDebug() << "cancel";
    _topWordsModel->resetTopWords({});
    _progress = 0;
    _isPaused = false;
    _isRunning = false;
    emit progressChanged();
    emit readingCancel();

    emit pausedChanged();
    emit runningChanged();
}

void WordPulseViewModel::showInfo(const QString &msg)
{
    emit systemMessage(MsgInfo, msg);
}

void WordPulseViewModel::showWarning(const QString &msg)
{
    emit systemMessage(MsgWarning, msg);
}

void WordPulseViewModel::showError(const QString &msg)
{
    emit systemMessage(MsgError, msg);
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
    if (_isPaused)
        return;

    if (this->_progress != progress) {
        this->_progress = progress;
        qDebug() << "progressChanged";
        emit progressChanged();
    }
}

void WordPulseViewModel::updateTopWords(const QVector<QPair<quint64, QString>>& newTopWords) {
    if (_isPaused)
        return;

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
        _topWordsModel->resetTopWords(newTopWords);
    }
}

void WordPulseViewModel::finishProcess()
{
    _isRunning = false;
    emit runningChanged();
    emit showInfo("Анализ завершён!");
}

TopWordsModel* WordPulseViewModel::getTopWordsModel() const noexcept {
    return _topWordsModel;
}
