#include <QtTest>
#include <QSignalSpy>
#include <memory>
#include "../src/blockanalyzerthread.h"
#include "../src/config.h"
#include "mockdataprovider.h"

class TestBlockAnalyzer : public QObject
{
    Q_OBJECT

private slots:
    void testQueueProcessingAndSignals() {
        Config cfg = Config::defaultConfig();
        cfg.top_n = 10;

        MockDataProvider mock;
        mock.addData("block one");
        mock.addData("block two");
        mock.addData("block three");

        // 1. Создаем smart pointer
        auto analyzer = std::make_unique<BlockAnalyzerThread>(cfg, &mock);
        analyzer->setTotalSize(500);

        // 2. Для Spy берем сырой указатель через .get()
        QSignalSpy spyTrigger(analyzer.get(), &BlockAnalyzerThread::thresholdBlockFreed);

        analyzer->start();
        QMetaObject::invokeMethod(analyzer.get(), "startAnalyzis", Qt::QueuedConnection);

        // Вызываем обработку
        QMetaObject::invokeMethod(analyzer.get(), "analyzeBlock", Qt::QueuedConnection);
        QMetaObject::invokeMethod(analyzer.get(), "analyzeBlock", Qt::QueuedConnection);
        QMetaObject::invokeMethod(analyzer.get(), "analyzeBlock", Qt::QueuedConnection);

        QTest::qWait(100);

        QVERIFY(mock.isDataEmpty());

        QThread* mainThread = QThread::currentThread();
        QMetaObject::invokeMethod(analyzer.get(), [analyzer = analyzer.get(), mainThread]() {
            analyzer->moveToThread(mainThread);
        }, Qt::BlockingQueuedConnection);

        // Очистка памяти и остановка потока
        analyzer->quit();
        analyzer->wait();
    }

    void testTopNLimit() {
        Config cfg = Config::defaultConfig();
        cfg.top_n = 3;

        MockDataProvider mock;
        QString input = "A A A A A B B B B C C C D D E";
        mock.addData(input.toUtf8());

        // Smart pointer
        auto analyzer = std::make_unique<BlockAnalyzerThread>(cfg, &mock);
        analyzer->setTotalSize(input.size());

        QSignalSpy spy(analyzer.get(), &BlockAnalyzerThread::topWords);

        analyzer->start();
        QMetaObject::invokeMethod(analyzer.get(), "startAnalyzis", Qt::QueuedConnection);
        QMetaObject::invokeMethod(analyzer.get(), "analyzeBlock", Qt::QueuedConnection);

        QVERIFY2(spy.wait(1000), "Timeout");

        auto list = spy.takeLast().at(0).value<QVector<QPair<quint64, QString>>>();

        QCOMPARE(list.size(), 3);

        // Проверки (предполагаем case_sensitive = false, значит toLower)
        QCOMPARE(list[2].second, QString("a"));
        QCOMPARE(list[2].first, 5ULL);
        QCOMPARE(list[1].second, QString("b"));
        QCOMPARE(list[1].first, 4ULL);
        QCOMPARE(list[0].second, QString("c"));
        QCOMPARE(list[0].first, 3ULL);

        QThread* mainThread = QThread::currentThread();
        QMetaObject::invokeMethod(analyzer.get(), [analyzer = analyzer.get(), mainThread]() {
            analyzer->moveToThread(mainThread);
        }, Qt::BlockingQueuedConnection);

        // Очистка памяти и остановка потока
        analyzer->quit();
        analyzer->wait();
    }

    void testResetLogic() {
        Config cfg = Config::defaultConfig();
        cfg.top_n = 5;
        MockDataProvider mock;

        auto analyzer = std::make_unique<BlockAnalyzerThread>(cfg, &mock);
        analyzer->setTotalSize(1000);
        analyzer->start();
        QMetaObject::invokeMethod(analyzer.get(), "startAnalyzis", Qt::QueuedConnection);

        // 1. Считаем "Hello"
        mock.addData("Hello");
        QMetaObject::invokeMethod(analyzer.get(), "analyzeBlock", Qt::QueuedConnection);

        // 2. Сброс
        QMetaObject::invokeMethod(analyzer.get(), "cancelAnalyzis", Qt::QueuedConnection);
        QTest::qWait(100);

        QSignalSpy spy(analyzer.get(), &BlockAnalyzerThread::topWords);
        // 3. Считаем "World"
        mock.addData("World");
        QMetaObject::invokeMethod(analyzer.get(), "startAnalyzis", Qt::QueuedConnection);
        QMetaObject::invokeMethod(analyzer.get(), "analyzeBlock", Qt::QueuedConnection);

        // 4. Проверка

        //QMetaObject::invokeMethod(analyzer.get(), "emitUpdate", Qt::QueuedConnection);

        QVERIFY(spy.wait(1000));

        auto list = spy.takeLast().at(0).value<QVector<QPair<quint64, QString>>>();

        QCOMPARE(list.size(), 1);
        QCOMPARE(list[0].second, QString("world"));

        QThread* mainThread = QThread::currentThread();
        QMetaObject::invokeMethod(analyzer.get(), [analyzer = analyzer.get(), mainThread]() {
            analyzer->moveToThread(mainThread);
        }, Qt::BlockingQueuedConnection);

        // Очистка памяти и остановка потока
        analyzer->quit();
        analyzer->wait();
    }

    void testSimpleCounting() {
        // 1. Настройка
        Config cfg = Config::defaultConfig();
        cfg.top_n = 5;

        MockDataProvider mock;
        QByteArray data = "hello world hello"; // Создаем данные отдельно, чтобы знать размер
        mock.addData(data);

        std::unique_ptr<BlockAnalyzerThread> analyzer = std::make_unique<BlockAnalyzerThread>(cfg, &mock);

        analyzer->setTotalSize(data.size());

        QSignalSpy spy(analyzer.get(), &BlockAnalyzerThread::topWords);

        // 2. ЗАПУСК ПОТОКА (Теперь это обязательно)
        analyzer->start();

        QMetaObject::invokeMethod(analyzer.get(), "startAnalyzis", Qt::QueuedConnection);

        QMetaObject::invokeMethod(analyzer.get(), "analyzeBlock", Qt::QueuedConnection);

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

        QThread* mainThread = QThread::currentThread();
        QMetaObject::invokeMethod(analyzer.get(), [analyzer = analyzer.get(), mainThread]() {
            analyzer->moveToThread(mainThread);
        }, Qt::BlockingQueuedConnection);

        // Очистка памяти и остановка потока
        analyzer->quit();
        analyzer->wait();
    }

    void testSpecialCharacters() {
        Config cfg = Config::defaultConfig();
        MockDataProvider mock;
        QByteArray data = "run, run! run...";
        mock.addData(data);

        std::unique_ptr<BlockAnalyzerThread> analyzer = std::make_unique<BlockAnalyzerThread>(cfg, &mock);

        analyzer->setTotalSize(data.size());
        mock.addData(data);

        QSignalSpy spy(analyzer.get(), &BlockAnalyzerThread::topWords);

        analyzer->start();

        QMetaObject::invokeMethod(analyzer.get(), "startAnalyzis", Qt::QueuedConnection);

        QMetaObject::invokeMethod(analyzer.get(), "analyzeBlock", Qt::QueuedConnection);

        QVERIFY2(spy.wait(1000), "Timeout waiting for topWords signal");
        QCOMPARE(spy.count(), 1);
        auto list = spy.takeFirst().at(0).value<QVector<QPair<quint64, QString>>>();

        QCOMPARE(list.size(), 1);
        QCOMPARE(list[0].second, QString("run"));
        QCOMPARE(list[0].first, 3ULL);

        QThread* mainThread = QThread::currentThread();
        QMetaObject::invokeMethod(analyzer.get(), [analyzer = analyzer.get(), mainThread]() {
            analyzer->moveToThread(mainThread);
        }, Qt::BlockingQueuedConnection);

        // Очистка памяти и остановка потока
        analyzer->quit();
        analyzer->wait();
    }

    void testCaseSensitivity() {
        // 1. Настройка
        Config cfg = Config::defaultConfig();
        cfg.top_n = 5;
        // ВАЖНО: Включаем чувствительность к регистру
        cfg.case_sensitive = true;

        MockDataProvider mock;
        // Три разных варианта написания
        QByteArray data = "Hello hello HELLO";
        mock.addData(data);

        std::unique_ptr<BlockAnalyzerThread> analyzer = std::make_unique<BlockAnalyzerThread>(cfg, &mock);
        analyzer->setTotalSize(data.size());

        QSignalSpy spy(analyzer.get(), &BlockAnalyzerThread::topWords);

        // 2. Запуск
        analyzer->start();
        QMetaObject::invokeMethod(analyzer.get(), "startAnalyzis", Qt::QueuedConnection);
        QMetaObject::invokeMethod(analyzer.get(), "analyzeBlock", Qt::QueuedConnection);

        // 3. Проверка
        QVERIFY2(spy.wait(1000), "Timeout waiting for topWords signal");
        QCOMPARE(spy.count(), 1);

        auto list = spy.takeFirst().at(0).value<QVector<QPair<quint64, QString>>>();

        // Ожидаем 3 разных слова (так как регистр учитывается)
        QCOMPARE(list.size(), 3);

        // Собираем найденные слова в список для проверки
        QStringList wordsFound;
        for (const auto& pair : list) {
            // Каждое слово должно встретиться ровно 1 раз
            QCOMPARE(pair.first, 1ULL);
            wordsFound << pair.second;
        }

        // Проверяем наличие всех вариантов
        QVERIFY(wordsFound.contains("Hello"));
        QVERIFY(wordsFound.contains("hello"));
        QVERIFY(wordsFound.contains("HELLO"));

        QThread* mainThread = QThread::currentThread();
        QMetaObject::invokeMethod(analyzer.get(), [analyzer = analyzer.get(), mainThread]() {
            analyzer->moveToThread(mainThread);
        }, Qt::BlockingQueuedConnection);

        // Очистка памяти и остановка потока
        analyzer->quit();
        analyzer->wait();
    }
};

// Этот макрос создает main() функцию для запуска тестов
QTEST_MAIN(TestBlockAnalyzer)
#include "tst_analyzer.moc"
