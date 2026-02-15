#include "wordpulseviewmodel.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include "topwordsmodel.h"
#include "analyzerorchestrator.h"

WordPulseViewModel::WordPulseViewModel(QObject *parent)
    : QObject(parent), _configPath("config.json"), _config(Config::fromJson(_configPath))
{
    _topWordsModel = new TopWordsModel(this);
    _progress = 0;
    _isRunning = false;
    _isPaused = false;
    _fileChosen = false;
}

WordPulseViewModel::~WordPulseViewModel() {
    cleanupOrchestrator();
}

void WordPulseViewModel::openFile() {
    QString fileName = QFileDialog::getOpenFileName(
        nullptr, tr("Открыть файл"), QDir::homePath(), tr("Текстовые файлы (*.txt *.log);;Все файлы (*)")
        );

    if (fileName.isEmpty()) {
        showWarning("Выбор файла отменён");
        return;
    }

    _currentFilePath = fileName;
    QFileInfo fileInfo(fileName);

    _topWordsModel->resetTopWords({});
    setProgress(0);
    _fileChosen = true;

    showInfo(QString("Выбран файл: %1 (%2 КБ)").arg(fileInfo.fileName()).arg(fileInfo.size() / 1024));
}

void WordPulseViewModel::start() {
    if (!_fileChosen) { showWarning("Файл не выбран!"); return; }

    cleanupOrchestrator(); // Полностью чистим всё старое

    _topWordsModel->resetTopWords({});
    setProgress(0);

    // Создаем голый поток и объект Оркестратора
    _orchestratorThread = new QThread(this);
    _orchestrator = new AnalyzerOrchestrator(_config, _currentFilePath);

    // Переносим объект в фоновый поток. Теперь все его слоты и таймеры будут жить там!
    _orchestrator->moveToThread(_orchestratorThread);

    // При старте потока - запускаем анализ
    connect(_orchestratorThread, &QThread::started, _orchestrator, &AnalyzerOrchestrator::startProcessing);

    // Привязки UI
    connect(_orchestrator, &AnalyzerOrchestrator::sigTopWordsReady, this, &WordPulseViewModel::onTopWordsReady);
    connect(_orchestrator, &AnalyzerOrchestrator::sigProgressUpdated, this, &WordPulseViewModel::onProgressUpdated);
    connect(_orchestrator, &AnalyzerOrchestrator::sigFinished, this, &WordPulseViewModel::onAnalysisFinished);
    connect(_orchestrator, &AnalyzerOrchestrator::sigError, this, &WordPulseViewModel::onAnalysisError);
    connect(_orchestrator, &AnalyzerOrchestrator::sigCanceled, this, &WordPulseViewModel::onAnalysisCanceled);

    // Запускаем!
    _orchestratorThread->start();

    _isRunning = true;
    _isPaused = false;
    emit runningChanged();
    emit pausedChanged();
}

void WordPulseViewModel::pause() {
    _isPaused = true;
    if (_orchestrator) _orchestrator->pause();
    emit pausedChanged();
}

void WordPulseViewModel::resume() {
    _isPaused = false;
    if (_orchestrator) _orchestrator->resume();
    emit pausedChanged();
}

void WordPulseViewModel::cancel() {
    showInfo("Отмена операции...");

    if (_orchestrator) {
        QMetaObject::invokeMethod(_orchestrator, "cancelProcessing", Qt::QueuedConnection);
    }
}

// ЭТО САМЫЙ БЕЗОПАСНЫЙ СПОСОБ ЗАКРЫТЬ ПОТОК В C++
void WordPulseViewModel::cleanupOrchestrator() {
    if (_orchestrator && _orchestratorThread) {
        _orchestrator->requestCancel();

        QMetaObject::invokeMethod(_orchestrator, "performCleanup", Qt::BlockingQueuedConnection);

        _orchestratorThread->quit();
        _orchestratorThread->wait();

        delete _orchestrator;
        delete _orchestratorThread;

        _orchestrator = nullptr;
        _orchestratorThread = nullptr;
    }
}

void WordPulseViewModel::onTopWordsReady(const QVector<QPair<quint64, QString>>& topWords) {
    _topWordsModel->resetTopWords(topWords);
}

void WordPulseViewModel::onProgressUpdated(quint8 progress) {
    setProgress(progress);
}

void WordPulseViewModel::onAnalysisFinished() {
    cleanupOrchestrator();
    _isRunning = false;
    emit runningChanged();
    setProgress(100);
    showInfo("Анализ успешно завершён!");
}

void WordPulseViewModel::onAnalysisError(const QString& error) {
    showError(error);
    cancel();
}

void WordPulseViewModel::onAnalysisCanceled() {
    cleanupOrchestrator();

    _topWordsModel->resetTopWords({});
    setProgress(0);

    _isRunning = false;
    _isPaused = false;
    emit runningChanged();
    emit pausedChanged();
}

void WordPulseViewModel::setProgress(quint8 progress) {
    if (_progress != progress) {
        _progress = progress;
        emit progressChanged();
    }
}
