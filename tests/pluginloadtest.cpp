#include <core/plugins/plugin.h>

#include <QJsonObject>
#include <QPluginLoader>
#include <QtTest>

class PluginLoadTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loads()
    {
        const QString path = qEnvironmentVariable("DRMETER_PLUGIN_PATH");
        QVERIFY2(!path.isEmpty(), "DRMETER_PLUGIN_PATH is missing");

        QPluginLoader loader{path};
        QCOMPARE(loader.metaData().value(QStringLiteral("IID")).toString(),
                 QStringLiteral("org.fooyin.fooyin.plugin/1.0"));
        QCOMPARE(loader.metaData().value(QStringLiteral("MetaData")).toObject().value(QStringLiteral("Name")).toString(),
                 QStringLiteral("Dynamic Range Meter"));

        QObject* instance = loader.instance();
        QVERIFY2(instance, qPrintable(loader.errorString()));
        QVERIFY(qobject_cast<Fooyin::Plugin*>(instance));
        QVERIFY(loader.unload());
    }
};

QTEST_GUILESS_MAIN(PluginLoadTest)

#include "pluginloadtest.moc"
