/**
 * Copyright (c) 2009, Benjamin C. Meyer  <ben@meyerhome.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "autofillmanager.h"

#include "browserapplication.h"
#include "browsermainwindow.h"

#include <qdebug.h>
#include <qdesktopservices.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qsettings.h>
#include <qurl.h>
#include <qwebframe.h>
#include <qwebpage.h>

// #define AUTOFILL_DEBUG

// TODO make model on top
AutoFillManager::AutoFillManager(QObject *parent)
    : QObject(parent)
    , m_enabled(true)
{
    load();
    QSettings settings;
    settings.beginGroup(QLatin1String("AutoFill"));
    m_enabled = settings.value(QLatin1String("enabled"), m_enabled).toBool();
}

AutoFillManager::~AutoFillManager()
{
    save();
}

// TODO for a kde/gnome/osx/backend?
void AutoFillManager::store(const QUrl &url, const QByteArray &data)
{
    // TODO
    // store multiple data for one url
    // separate passwords from normal forms?
#ifdef AUTOFILL_DEBUG
    qDebug() << "Saving" << url << data;
#endif
    m_data.insert(url.toEncoded(), data);
}
/*
static rot13(const QByteArray data)
{
    QByteArray r;
    for (int i = 0; i < data.count(); ++i)
        r += data[i] + 13;
    return r;
}*/

QString AutoFillManager::autoFillDataFile()
{
    QString fileName = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    fileName += QLatin1String("/autofill.dat");
    return fileName;
}

void AutoFillManager::save()
{
    QString fileName = autoFillDataFile();
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        qWarning() << "Unable to open" << fileName << "to store autofill data";
        return;
    }

    QTextStream stream(&file);
    QHashIterator<QByteArray, QByteArray> i(m_data);
    while (i.hasNext()) {
        i.next();
        stream << i.key() << endl << i.value() << endl;
    }
}

void AutoFillManager::load()
{
    QString fileName = autoFillDataFile();
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return;

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QByteArray key = stream.readLine().toUtf8();
        if (stream.atEnd())
            break;
        QByteArray value = stream.readLine().toUtf8();
        m_data.insert(key, value);
    }
}

void AutoFillManager::post(const QNetworkRequest &request, const QByteArray &outgoingData)
{
#ifdef AUTOFILL_DEBUG
    qDebug() << "AutoFillManager::" << __FUNCTION__ << outgoingData << request.url();
#endif
    if (!m_enabled)
        return;

    QByteArray refererHeader = request.rawHeader("Referer");
    if (refererHeader.isEmpty())
        return;
    QUrl url = QUrl::fromEncoded(refererHeader);
    url = stripUrl(url);

    if (m_never.contains(url))
        return;

    // TODO Determine if it is a password by checking the dom rather then this hack
    bool password = outgoingData.contains("&pass");
    if (password && !m_data.contains(url.toEncoded())) {
        QMessageBox *messageBox = new QMessageBox(BrowserApplication::instance()->mainWindow());
        messageBox->setText(tr("<b>Would you like to save this password?</b><br> \
        To review passwords you have saved and remove them, open the AutoFill pane of preferences."));
        messageBox->addButton(tr("Never for this site"), QMessageBox::DestructiveRole);
        messageBox->addButton(tr("Not now"), QMessageBox::RejectRole);
        messageBox->addButton(QMessageBox::Yes);
        messageBox->setDefaultButton(QMessageBox::Yes);
        switch (messageBox->exec()) {
        case QMessageBox::DestructiveRole:
            m_never.append(url);
            // fall through
        case QMessageBox::RejectRole:
            return;
        default:
            break;
        }
        messageBox->deleteLater();
    }
    store(url, outgoingData);
}

QUrl AutoFillManager::stripUrl(const QUrl &url)
{
    QUrl cleanUrl = url;
    cleanUrl.setQueryItems(QList<QPair<QString, QString> >());
    cleanUrl.setFragment(QString());
    cleanUrl.setUserInfo(QString());
    return cleanUrl;
}

void AutoFillManager::fill(QWebPage *page)
{
#ifdef AUTOFILL_DEBUG
    qDebug() << "AutoFillManager::" << __FUNCTION__ << page;
#endif
    if (!m_enabled)
        return;
    if (!page || !page->mainFrame())
        return;

    QUrl url = page->mainFrame()->url();
    url = stripUrl(url);

    if (!m_data.contains(url.toEncoded()))
        return;

    QByteArray data = m_data[url.toEncoded()];
    if (data.isEmpty())
        return;

    QUrl argsUrl = QUrl::fromEncoded(QByteArray("foo://bar.com/?" + data));
    QList<QPair<QString, QString> > args = argsUrl.queryItems();
    // crude and ugly and will happily change this if you know of a better way
    // version 1 and for what it is worth works no matter how crude it is
    int formCount = page->mainFrame()->evaluateJavaScript(QLatin1String("document.forms.length")).toInt();
    qDebug() << "Filling for url:" << url << m_data[url.toEncoded()];
    for (int i = 0; i < args.count(); ++i) {
        QString key = args[i].first;
        QString value = args[i].second;
        for (int j = 0; j < formCount; ++j) {
            int formIndex = j;
            bool disabled = page->mainFrame()->evaluateJavaScript(QString(QLatin1String("document.forms[%1].%2.disabled")).arg(formIndex).arg(key)).toBool();
            if (disabled)
                continue;
            bool readOnly = page->mainFrame()->evaluateJavaScript(QString(QLatin1String("document.forms[%1].%2.readonly")).arg(formIndex).arg(key)).toBool();
            if (readOnly)
                continue;

            QString type = page->mainFrame()->evaluateJavaScript(QString(QLatin1String("document.forms[%1].%2.type")).arg(formIndex).arg(key)).toString();
            if (type.isEmpty()
                || type == QLatin1String("hidden")
                || type == QLatin1String("reset")
                || type == QLatin1String("submit") )
                continue;
            qDebug() << "type:" << type << "readonly" << readOnly << "disabled" << disabled;
            QString javascript = QString(QLatin1String("document.forms[%1].%2.value=\"%3\";"))
                    .arg(formIndex)
                    .arg(key)
                    .arg(value);
            page->mainFrame()->evaluateJavaScript(javascript);
        }
    }
}

