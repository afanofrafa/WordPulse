#include "config.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>
#include <QDir>

Config Config::fromJson(const QString& path) {
    QFile file(path);
    qInfo() << "Loading config from:" << QFileInfo(path).absoluteFilePath();

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open config file (" << path << "), using defaults.";
        return defaultConfig();
    }

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCritical() << "Config JSON parse error:" << error.errorString() << "Offset:" << error.offset;
        return defaultConfig();
    }

    if (!doc.isObject())
        return defaultConfig();

    auto obj = doc.object();

    Config cfg;
    cfg.top_n = obj.value("top_n").toInt(15);
    cfg.max_chunks_in_mem_num = obj.value("max_chunks_in_mem_num").toInt(10);
    cfg.update_interval_ms = obj.value("update_interval_ms").toInt(1);
    qDebug() << "upd int" << cfg.update_interval_ms;
    cfg.chunk_size_bytes = obj.value("chunk_size_bytes").toInteger(1024 * 128);
    cfg.string_pattern = obj.value("word_pattern").toString("\\w+");
    cfg.case_sensitive = obj.value("case_sensitive").toBool(false);
    QString sepStr = obj.value("word_separators").toString(" \t\n\r.,!?;:'\"()[]{}<>-—–/\\|&*@#%^+=~`");
    cfg.word_separators.clear();
    for (QChar ch : sepStr) {
        if (ch.unicode() <= 127) {  // только ASCII разделители
            cfg.word_separators.insert(static_cast<char>(ch.unicode()));
        }
    }

    // Валидация
    if (cfg.top_n <= 0 || cfg.chunk_size_bytes <= 0
        || cfg.max_chunks_in_mem_num <= 0 || cfg.update_interval_ms <= 0)
    {
        qWarning() << "Invalid top_n in config:" << cfg.top_n << ". Using default.";
        return defaultConfig();
    }

    // Проверка regex
    QRegularExpression re(cfg.string_pattern);
    if (!re.isValid()) {
        qInfo() << "Config loaded successfully. Pattern:" << cfg.string_pattern << "TopN:" << cfg.top_n;
        return defaultConfig();
    }

    return cfg;
}

Config Config::defaultConfig()
{
    Config cfg;
    cfg.top_n = 15;
    cfg.max_chunks_in_mem_num = 10;
    cfg.update_interval_ms = 1;
    cfg.chunk_size_bytes = 1024 * 128;
    cfg.string_pattern = "\\w+";
    cfg.case_sensitive = false;
    const char* defaults = " \t\n\r.,!?;:'\"()[]{}<>-—–/\\|&*@#%^+=~`";
    cfg.word_separators.clear();
    for (const char* p = defaults; *p; ++p) {
        cfg.word_separators.insert(*p);
    }
    return cfg;
}
