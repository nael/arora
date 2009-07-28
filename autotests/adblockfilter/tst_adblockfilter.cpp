/*
 * Copyright 2009 Benjamin C. Meyer <ben@meyerhome.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include <QtTest/QtTest>
#include <adblockfilter.h>

class tst_AdBlockFilter : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void match_data();
    void match();
};

// Subclass that exposes the protected functions.
class SubAdBlockFilter : public AdBlockFilter
{
public:
    SubAdBlockFilter(const QString &filter)
       : AdBlockFilter(filter) {}
};

// This will be called before the first test function is executed.
// It is only called once.
void tst_AdBlockFilter::initTestCase()
{
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_AdBlockFilter::cleanupTestCase()
{
}

// This will be called before each test function is executed.
void tst_AdBlockFilter::init()
{
}

// This will be called after every test function.
void tst_AdBlockFilter::cleanup()
{
}

void tst_AdBlockFilter::match_data()
{
    QTest::addColumn<QString>("filter");
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("match");
    QTest::newRow("null") << QString() << QUrl() << false;

    //Examples taken from http://adblockplus.org/en/filters

    // Basic filter rules
    QTest::newRow("b0") << QString("http://example.com/ads/banner123.gif")
                        << QUrl("http://example.com/ads/banner123.gif")
                        << true;
    QTest::newRow("b1") << QString("http://example.com/ads/banner*.gif")
                        << QUrl("http://example.com/ads/banner123.gif")
                        << true;
    QTest::newRow("b2") << QString("http://example.com/ads/*")
                        << QUrl("http://example.com/ads/banner123.gif")
                        << true;
    QTest::newRow("b3") << QString("http://example.com/*")
                        << QUrl("http://example.com/ads/banner123.gif")
                        << true;

    // defining exception rules
    QTest::newRow("e0") << QString("@@advice")
                        << QUrl("http://example.com/advice.html")
                        << true;
    QTest::newRow("e1") << QString("@@|http://example.com")
                        << QUrl("http://example.com/advice.html")
                        << true;
    QTest::newRow("e2") << QString("@@http://example.com")
                        << QUrl("http://example.com/advice.html")
                        << true;

    // matching at beginning/end of an address
    QTest::newRow("m0") << QString("ad")
                        << QUrl("http://example.com/advice.html")
                        << true;
    QTest::newRow("m1") << QString("*ad*")
                        << QUrl("http://example.com/advice.html")
                        << true;
    QTest::newRow("m2") << QString("swf")
                        << QUrl("http://example.com/swf/index.html")
                        << true;
    QTest::newRow("m3") << QString("swf|")
                    << QUrl("http://example.com/annoyingflash.swf")
                    << true;
    QTest::newRow("m4") << QString("swf|")
                    << QUrl("http://example.com/swf/index.html")
                    << false;

    QTest::newRow("m5") << QString("http://baddomain.example/")
                    << QUrl("http://baddomain.example/banner.gif")
                    << true;
    QTest::newRow("m6") << QString("http://baddomain.example/")
                    << QUrl("http://gooddomain.example/analyze?http://baddomain.example.")
                    << false;

    QTest::newRow("m5") << QString("||example.com/banner.gif")
                    << QUrl("http://example.com/banner.gif")
                    << true;
    QTest::newRow("m5") << QString("||example.com/banner.gif")
                    << QUrl("https://example.com/banner.gif")
                    << true;
    QTest::newRow("m5") << QString("||example.com/banner.gif")
                    << QUrl("http://www.example.com/banner.gif")
                    << true;

    QTest::newRow("m5") << QString("||example.com/banner.gif")
                    << QUrl("http://badexample.com/banner.gif")
                    << false;
    QTest::newRow("m5") << QString("||example.com/banner.gif")
                    << QUrl("http://gooddomain.example/analyze?http://example.com/banner.gif")
                    << false;

}

// public bool match(const QUrl &url) const
void tst_AdBlockFilter::match()
{
    QFETCH(QString, filter);
    QFETCH(QUrl, url);
    QFETCH(bool, match);

    SubAdBlockFilter adBlockFilter(filter);

    QCOMPARE(adBlockFilter.match(url), match);
}

QTEST_MAIN(tst_AdBlockFilter)
#include "tst_adblockfilter.moc"

