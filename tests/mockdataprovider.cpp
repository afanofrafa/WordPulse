#include "mockdataprovider.h"

void MockDataProvider::addData(const QString &str)
{
    queue.enqueue(str.toUtf8());
}

bool MockDataProvider::isDataEmpty() const noexcept
{
    return queue.isEmpty();
}

qsizetype MockDataProvider::dataSize() const noexcept
{
    return 0;
}

QByteArrayView MockDataProvider::getDataBlock()
{
    if (queue.isEmpty()) return QByteArrayView();

    currentData = queue.dequeue();

    return currentData;
}
