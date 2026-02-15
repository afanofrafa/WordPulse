#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <array> // <-- Добавляем инклюд

struct Config {
    qint32 top_n;
    qint32 update_interval_ms;
    qint32 num_threads;
    bool case_sensitive;

    // Наша сверхбыстрая таблица разделителей
    std::array<bool, 256> is_delim_table{false};

    static Config fromJson(const QString& path);
    static Config defaultConfig();
};

#endif // CONFIG_H
