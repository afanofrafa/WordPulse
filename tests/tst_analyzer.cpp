#include <QtTest>
#include <QSignalSpy>
#include "../src/blockanalyzerthread.h"
#include "../src/config.h"
#include "mockdataprovider.h"

class TestBlockAnalyzer : public QObject
{
    Q_OBJECT

private slots:
    void testSimpleCounting() {
        // 1. Настройка
        Config cfg = Config::defaultConfig();
        cfg.top_n = 5;

        MockDataProvider mock;
        QByteArray data = "hello world hello"; // Создаем данные отдельно, чтобы знать размер
        mock.addData(data);

        BlockAnalyzerThread* analyzer = new BlockAnalyzerThread(cfg, &mock);

        analyzer->setTotalSize(data.size());

        QSignalSpy spy(analyzer, &BlockAnalyzerThread::topWords);

        // 2. ЗАПУСК ПОТОКА (Теперь это обязательно)
        analyzer->start();

        QMetaObject::invokeMethod(analyzer, "startAnalyzis", Qt::QueuedConnection);

        QMetaObject::invokeMethod(analyzer, "analyzeBlock", Qt::QueuedConnection);

        // 3. Проверка
        QVERIFY2(spy.wait(1000), "Timeout waiting for topWords signal");
        QCOMPARE(spy.count(), 1);

        QList<QVariant> arguments = spy.takeFirst();

        auto list = arguments.at(0).value<QVector<QPair<quint64, QString>>>();

        QVERIFY(!list.isEmpty());

        auto it = list.begin();
        QCOMPARE(it->second, QString("world"));
        QCOMPARE(it->first, 1ULL);
        ++it;
        QCOMPARE(it->second, QString("hello"));
        QCOMPARE(it->first, 2ULL);
    }

    void testSpecialCharacters() {
        Config cfg = Config::defaultConfig();
        MockDataProvider mock;
        QByteArray data = "run, run! run...";
        mock.addData(data);

        BlockAnalyzerThread* analyzer = new BlockAnalyzerThread(cfg, &mock);

        analyzer->setTotalSize(data.size());
        mock.addData(data);

        QSignalSpy spy(analyzer, &BlockAnalyzerThread::topWords);

        analyzer->start();

        QMetaObject::invokeMethod(analyzer, "startAnalyzis", Qt::QueuedConnection);

        QMetaObject::invokeMethod(analyzer, "analyzeBlock", Qt::QueuedConnection);

        QVERIFY2(spy.wait(1000), "Timeout waiting for topWords signal");
        QCOMPARE(spy.count(), 1);
        auto list = spy.takeFirst().at(0).value<QVector<QPair<quint64, QString>>>();

        QCOMPARE(list.size(), 1);
        QCOMPARE(list[0].second, QString("run"));
        QCOMPARE(list[0].first, 3ULL);
    }
};

// Этот макрос создает main() функцию для запуска тестов
QTEST_MAIN(TestBlockAnalyzer)
#include "tst_analyzer.moc"
