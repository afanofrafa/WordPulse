#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QThread>

class Logger {
public:
    static void init(const QString& logPath);

private:
    QFile logFile;
    static Logger* self;
    QMutex mutex;

    Logger(const QString& path);
    ~Logger();

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
};

#endif // LOGGER_H
