#ifndef IDATAPROVIDER_H
#define IDATAPROVIDER_H

#include <QByteArray>
#include <QMutex>

class IDataProvider {
public:
    virtual ~IDataProvider() = default;

    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual bool isDataEmpty() const = 0;
    virtual qsizetype dataSize() const = 0;
    virtual QByteArrayView getDataBlock() = 0;
};

#endif // IDATAPROVIDER_H
