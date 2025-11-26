#include "mockdataprovider.h"

void MockDataProvider::addData(const QString &str)
{
    queue.enqueue(str.toUtf8());
}

bool MockDataProvider::isDataEmpty() const
{
    return queue.isEmpty();
}

QByteArrayView MockDataProvider::getDataBlock()
{
    if (queue.isEmpty()) return QByteArrayView();

    currentData = queue.dequeue();

    return currentData;
}
