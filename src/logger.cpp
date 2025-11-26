#include "logger.h"
#include <iostream>

Logger* Logger::self = nullptr;

void Logger::init(const QString &logPath)
{
    static Logger instance(logPath);
    qInstallMessageHandler(Logger::messageHandler);
}

Logger::Logger(const QString &path)
{
    logFile.setFileName(path);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        std::cerr << "Could not open log file: " << path.toStdString() << std::endl;
    }
    self = this;
}

Logger::~Logger()
{
    if (logFile.isOpen())
        logFile.close();
}


void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (!self)
        return;

    QMutexLocker locker(&self->mutex);

    QString level;
    switch (type) {
    case QtDebugMsg:    level = "DEBUG"; break;
    case QtInfoMsg:     level = "INFO "; break;
    case QtWarningMsg:  level = "WARN "; break;
    case QtCriticalMsg: level = "CRIT "; break;
    case QtFatalMsg:    level = "FATAL"; break;
    }

    QString contextInfo;
    if (context.file && context.line) {
        contextInfo = QString(" [%1:%2]").arg(context.file).arg(context.line);
    }

    QString txt = QString("[%1] [Thread: %2] [%3]%4 %5")
                      .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                      .arg(reinterpret_cast<quint64>(QThread::currentThreadId()))
                      .arg(level)
                      .arg(contextInfo)
                      .arg(msg);

    if (self->logFile.isOpen()) {
        QTextStream stream(&self->logFile);
        stream << txt << "\n";
    }

    std::cout << txt.toLocal8Bit().constData() << std::endl;

    if (type == QtFatalMsg) abort();
}
