#include "topwordsmodel.h"

TopWordsModel::TopWordsModel(QObject* parent) : QAbstractListModel(parent) {
    maxCount = 0;
}

int TopWordsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return topWords.size();
}

QVariant TopWordsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= topWords.size())
        return QVariant();

    const auto& pair = topWords.at(index.row());

    switch (role) {
        case CountRole: return QVariant::fromValue(pair.first);
        case WordRole: return pair.second;
        default: return QVariant();
    }
}

QHash<int, QByteArray> TopWordsModel::roleNames() const {
    static QHash<int, QByteArray> roles = {
        {CountRole, "count"},
        {WordRole, "word"}
    };
    return roles;
}

void TopWordsModel::resetTopWords(const QVector<QPair<quint64, QString>>& newTopWords) {
    beginResetModel();
    topWords = newTopWords;
    quint64 newMax = calculateMax();
    if (newMax != maxCount) {
        maxCount = newMax;
        emit maxCountChanged();
    }
    endResetModel();
}

void TopWordsModel::updateWord(int index, quint64 newCount, const QString& word) {
    if (index < 0 || index >= topWords.size())
        return;
    auto& pair = topWords[index];
    bool countChanged = (newCount != pair.first);
    bool wordChanged = (!word.isEmpty() && word != pair.second);
    if (countChanged || wordChanged) {
        pair.first = newCount;
        if (!word.isEmpty()) pair.second = word;
        const QModelIndex idx = createIndex(index, 0);
        emit dataChanged(idx, idx, {CountRole, WordRole});

        if (countChanged) {
            quint64 newMax = calculateMax();
            if (newMax != maxCount) {
                maxCount = newMax;
                emit maxCountChanged();
            }
        }
    }
}
quint64 TopWordsModel::getMaxCount() const {
    return maxCount;
}

quint64 TopWordsModel::calculateMax() const {
    if (topWords.isEmpty()) return 0;
    quint64 max = 0;
    for (const auto& pair : topWords) {
        if (pair.first > max)
            max = pair.first;
    }
    return max;
}
