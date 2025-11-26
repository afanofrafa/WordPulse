#ifndef TOPWORDSMODEL_H
#define TOPWORDSMODEL_H

#include <QAbstractListModel>
#include <QPair>
#include <QVector>

class TopWordsModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(quint64 maxCount READ getMaxCount NOTIFY maxCountChanged)
public:
    explicit TopWordsModel(QObject* parent = nullptr);

    enum Roles {
        CountRole = Qt::UserRole + 1,
        WordRole
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    quint64 getMaxCount() const;
    // Полный сброс (использовать редко: start, cancel)
    void resetTopWords(const QVector<QPair<quint64, QString>>& newTopWords);

    // Частичное обновление (основной метод)
    void updateWord(int index, quint64 newCount, const QString& word = QString());  // word пустая = не менять
signals:
    void maxCountChanged();
private:
    QVector<QPair<quint64, QString>> topWords;  // {count, word}, sorted descending
    quint64 maxCount;  // ← Хранение максимума
    quint64 calculateMax() const;
};

#endif // TOPWORDSMODEL_H
