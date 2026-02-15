#include "config.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QDebug>
#include <QFileInfo>

Config Config::fromJson(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return defaultConfig();

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) return defaultConfig();

    auto obj = doc.object();
    Config cfg;
    cfg.top_n = obj.value("top_n").toInt(15);
    cfg.update_interval_ms = obj.value("update_interval_ms").toInt(100);
    cfg.case_sensitive = obj.value("case_sensitive").toBool(false);
    cfg.num_threads = obj.value("num_threads").toInt(QThread::idealThreadCount());
    if (cfg.num_threads <= 0) cfg.num_threads = 1;

    // Инициализируем нулями
    cfg.is_delim_table.fill(false);

    // Читаем разделители из JSON (с фоллбеком по умолчанию)
    QString sepStr = obj.value("word_separators").toString(" \t\n\r.,!?;:'\"()[]{}<>-—–/\\|&*@#%^+=~`");

    // Заполняем Look-up Table
    for (const QChar& ch : sepStr) {
        if (ch.unicode() <= 255) { // Защита от выхода за пределы массива
            cfg.is_delim_table[ch.unicode()] = true;
        }
    }

    return cfg;
}

Config Config::defaultConfig() {
    Config cfg;
    cfg.top_n = 15;
    cfg.update_interval_ms = 100;
    cfg.case_sensitive = false;
    cfg.num_threads = qMax(1, QThread::idealThreadCount());

    cfg.is_delim_table.fill(false);
    QString sepStr = " \t\n\r.,!?;:'\"()[]{}<>-—–/\\|&*@#%^+=~`";
    for (const QChar& ch : sepStr) {
        if (ch.unicode() <= 255) cfg.is_delim_table[ch.unicode()] = true;
    }

    return cfg;
}
