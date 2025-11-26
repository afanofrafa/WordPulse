#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <set>

struct Config {
    qint32 top_n;
    qint32 update_interval_ms;
    qint32 max_chunks_in_mem_num;
    qint64 chunk_size_bytes;
    QString string_pattern;
    bool case_sensitive;
    std::set<char> word_separators;

    static Config fromJson(const QString& path);
    static Config defaultConfig();
};

#endif // CONFIG_H
