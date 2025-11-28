#ifndef MOCKDATAPROVIDER_H
#define MOCKDATAPROVIDER_H

#include "../src/idataprovider.h"
#include <QQueue>
#include <QByteArray>

// Этот класс притворяется FileReader-ом
class MockDataProvider : public IDataProvider {
public:
    QQueue<QByteArray> queue;

    QByteArray currentData;

    void addData(const QString& str);

    void lock() override { }
    void unlock() override { }

    bool isDataEmpty() const noexcept override;

    qsizetype dataSize() const noexcept override;

    QByteArrayView getDataBlock() override;
};

#endif // MOCKDATAPROVIDER_H
