/*
 * Copyright 2008-2009 Benjamin C. Meyer <ben@meyerhome.net>
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

/****************************************************************************
**
** Copyright (C) 2007-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "networkaccessmanager.h"

#include "acceptlanguagedialog.h"
#include "autofillmanager.h"
#include "browserapplication.h"
#include "browsermainwindow.h"
#include "schemeaccesshandler.h"
#include "ui_passworddialog.h"
#include "ui_proxy.h"

#include <qdialog.h>
#include <qmessagebox.h>
#include <qsettings.h>
#include <qstyle.h>
#include <qtextdocument.h>

#include <qauthenticator.h>
#include <qdesktopservices.h>
#include <qnetworkdiskcache.h>
#include <qnetworkproxy.h>
#include <qnetworkreply.h>
#include <qsslconfiguration.h>
#include <qsslerror.h>
#include <qdatetime.h>

NetworkProxyFactory::NetworkProxyFactory()
    : QNetworkProxyFactory()
{
}

void NetworkProxyFactory::setHttpProxy(const QNetworkProxy &proxy)
{
    m_httpProxy = proxy;
}

void NetworkProxyFactory::setGlobalProxy(const QNetworkProxy &proxy)
{
    m_globalProxy = proxy;
}

QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> ret;

    if (query.protocolTag() == QLatin1String("http") && m_httpProxy.type() != QNetworkProxy::DefaultProxy)
        ret << m_httpProxy;
    ret << m_globalProxy;

    return ret;
}

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
    connect(this, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
            SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));
#ifndef QT_NO_OPENSSL
    connect(this, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            SLOT(sslErrors(QNetworkReply*, const QList<QSslError>&)));
#endif
    connect(BrowserApplication::instance(), SIGNAL(privacyChanged(bool)),
            this, SLOT(privacyChanged(bool)));
    loadSettings();

    // Register custom scheme handlers
    setSchemeHandler(QLatin1String("file"), new FileAccessHandler(this));
}

void NetworkAccessManager::setSchemeHandler(const QString &scheme, SchemeAccessHandler *handler)
{
    m_schemeHandlers.insert(scheme, handler);
}

void NetworkAccessManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("proxy"));
    QNetworkProxy proxy;
    if (settings.value(QLatin1String("enabled"), false).toBool()) {
        int proxyType = settings.value(QLatin1String("type"), 0).toInt();
        if (proxyType == 0)
            proxy = QNetworkProxy::Socks5Proxy;
        else if (proxyType == 1)
            proxy = QNetworkProxy::HttpProxy;
        else { // 2
            proxy.setType(QNetworkProxy::HttpCachingProxy);
            proxy.setCapabilities(QNetworkProxy::CachingCapability | QNetworkProxy::HostNameLookupCapability);
        }
        proxy.setHostName(settings.value(QLatin1String("hostName")).toString());
        proxy.setPort(settings.value(QLatin1String("port"), 1080).toInt());
        proxy.setUser(settings.value(QLatin1String("userName")).toString());
        proxy.setPassword(settings.value(QLatin1String("password")).toString());
    }
    NetworkProxyFactory *proxyFactory = new NetworkProxyFactory;
    if (proxy.type() == QNetworkProxy::HttpCachingProxy) {
        proxyFactory->setHttpProxy(proxy);
        proxyFactory->setGlobalProxy(QNetworkProxy::DefaultProxy);
    } else {
        proxyFactory->setHttpProxy(QNetworkProxy::DefaultProxy);
        proxyFactory->setGlobalProxy(proxy);
    }
    setProxyFactory(proxyFactory);
    settings.endGroup();

#ifndef QT_NO_OPENSSL
    QSslConfiguration sslCfg = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> ca_list = sslCfg.caCertificates();
    QList<QSslCertificate> ca_new = QSslCertificate::fromData(settings.value(QLatin1String("CaCertificates")).toByteArray());
    ca_list += ca_new;

    sslCfg.setCaCertificates(ca_list);
    QSslConfiguration::setDefaultConfiguration(sslCfg);
#endif

    settings.beginGroup(QLatin1String("network"));
    QStringList acceptList = settings.value(QLatin1String("acceptLanguages"),
            AcceptLanguageDialog::defaultAcceptList()).toStringList();
    m_acceptLanguage = AcceptLanguageDialog::httpString(acceptList);

    bool cacheEnabled = settings.value(QLatin1String("cacheEnabled"), true).toBool();
    if (QLatin1String(qVersion()) == QLatin1String("4.5.1"))
        cacheEnabled = false;

    if (cacheEnabled) {
        int maximumCacheSize = settings.value(QLatin1String("maximumCacheSize"), 50).toInt() * 1024 * 1024;

        QNetworkDiskCache *diskCache;
        if (cache())
            diskCache = qobject_cast<QNetworkDiskCache*>(cache());
        else
            diskCache = new QNetworkDiskCache(this);

        QString location = QDesktopServices::storageLocation(QDesktopServices::CacheLocation)
                                + QLatin1String("/browser");
        diskCache->setCacheDirectory(location);
        diskCache->setMaximumCacheSize(maximumCacheSize);
        setCache(diskCache);
    } else {
        if (QLatin1String(qVersion()) > QLatin1String("4.5.1"))
            setCache(0);
    }
    settings.endGroup();
}

void NetworkAccessManager::privacyChanged(bool isPrivate)
{
    if (isPrivate) {
        if (QLatin1String(qVersion()) > QLatin1String("4.5.1"))
            setCache(0);
    } else {
        loadSettings();
    }
}


void NetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *auth)
{
    BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    Ui::PasswordDialog passwordDialog;
    passwordDialog.setupUi(&dialog);

    passwordDialog.iconLabel->setText(QString());
    passwordDialog.iconLabel->setPixmap(mainWindow->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, mainWindow).pixmap(32, 32));

    QString introMessage = tr("<qt>Enter username and password for \"%1\" at %2</qt>");
    introMessage = introMessage.arg(Qt::escape(auth->realm())).arg(Qt::escape(reply->url().toString()));
    passwordDialog.introLabel->setText(introMessage);
    passwordDialog.introLabel->setWordWrap(true);

    if (dialog.exec() == QDialog::Accepted) {
        auth->setUser(passwordDialog.userNameLineEdit->text());
        auth->setPassword(passwordDialog.passwordLineEdit->text());
    }
}

void NetworkAccessManager::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth)
{
    BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    Ui::ProxyDialog proxyDialog;
    proxyDialog.setupUi(&dialog);

    proxyDialog.iconLabel->setText(QString());
    proxyDialog.iconLabel->setPixmap(mainWindow->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, mainWindow).pixmap(32, 32));

    QString introMessage = tr("<qt>Connect to proxy \"%1\" using:</qt>");
    introMessage = introMessage.arg(Qt::escape(proxy.hostName()));
    proxyDialog.introLabel->setText(introMessage);
    proxyDialog.introLabel->setWordWrap(true);

    if (dialog.exec() == QDialog::Accepted) {
        auth->setUser(proxyDialog.userNameLineEdit->text());
        auth->setPassword(proxyDialog.passwordLineEdit->text());
    }
}

#ifndef QT_NO_OPENSSL
QString NetworkAccessManager::certToFormattedString(QSslCertificate cert)
{
    QStringList message;
    message << cert.subjectInfo(QSslCertificate::CommonName);
    message << tr("Issuer: %1").arg(cert.issuerInfo(QSslCertificate::CommonName));
    message << tr("Not valid before: %1").arg(cert.effectiveDate().toString());
    message << tr("Valid until: %1").arg(cert.expiryDate().toString());

    QMultiMap<QSsl::AlternateNameEntryType, QString> names = cert.alternateSubjectNames();
    if (names.count() > 0) {
        QString list;
        list += QLatin1String("<br />");
        list += tr("Alternate Names:");
        list += QLatin1String("<ul><li>");
        list += QStringList(names.values(QSsl::DnsEntry)).join(QLatin1String("</li><li>"));
        list += QLatin1String("</li></ul>");
        message << list;
    }

    QString result = QLatin1String("<p>") + message.join(QLatin1String("<br />")) + QLatin1String("</p>");

    return result;
}

void NetworkAccessManager::sslErrors(QNetworkReply *reply, const QList<QSslError> &error)
{
    BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QSettings settings;
    QList<QSslCertificate> ca_merge = QSslCertificate::fromData(settings.value(QLatin1String("CaCertificates")).toByteArray());

    QList<QSslCertificate> ca_new;
    QStringList errorStrings;
    for (int i = 0; i < error.count(); ++i) {
        if (ca_merge.contains(error.at(i).certificate()))
            continue;
        errorStrings += error.at(i).errorString();
        if (!error.at(i).certificate().isNull()) {
            ca_new.append(error.at(i).certificate());
        }
    }
    if (errorStrings.isEmpty()) {
        reply->ignoreSslErrors();
        return;
    }

    QString errors = errorStrings.join(QLatin1String("</li><li>"));
    int ret = QMessageBox::warning(mainWindow,
                           QCoreApplication::applicationName() + tr(" - SSL Errors"),
                           tr("<qt>SSL Errors:"
                              "<br/><br/>for: <tt>%1</tt>"
                              "<ul><li>%2</li></ul>\n\n"
                              "Do you want to ignore these errors?</qt>").arg(reply->url().toString()).arg(errors),
                           QMessageBox::Yes | QMessageBox::No,
                           QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (ca_new.count() > 0) {
            QStringList certinfos;
            for (int i = 0; i < ca_new.count(); ++i)
                certinfos += certToFormattedString(ca_new.at(i));
            ret = QMessageBox::question(mainWindow, QCoreApplication::applicationName(),
                tr("<qt>Certificates:<br/>"
                   "%1<br/>"
                   "Do you want to accept all these certificates?</qt>")
                    .arg(certinfos.join(QString())),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                ca_merge += ca_new;

                QSslConfiguration sslCfg = QSslConfiguration::defaultConfiguration();
                QList<QSslCertificate> ca_list = sslCfg.caCertificates();
                ca_list += ca_new;
                sslCfg.setCaCertificates(ca_list);
                QSslConfiguration::setDefaultConfiguration(sslCfg);
                reply->setSslConfiguration(sslCfg);

                QByteArray pems;
                for (int i = 0; i < ca_merge.count(); ++i)
                    pems += ca_merge.at(i).toPem() + '\n';
                settings.setValue(QLatin1String("CaCertificates"), pems);
            }
        }
        reply->ignoreSslErrors();
    }
}
#endif

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    if (op == PostOperation && outgoingData) {
        QByteArray outgoingDataByteArray = outgoingData->peek(INT_MAX);
        BrowserApplication::autoFillManager()->post(request, outgoingDataByteArray);
    }

    QNetworkReply *reply = 0;

    // Check if there is a valid handler registered for the requested URL scheme
    if (m_schemeHandlers.contains(request.url().scheme()))
        reply = m_schemeHandlers[request.url().scheme()]->createRequest(op, request, outgoingData);
    if (reply)
        return reply;

    if (!m_acceptLanguage.isEmpty()) {
        QNetworkRequest req = request;
        req.setRawHeader("Accept-Language", m_acceptLanguage);
        reply = QNetworkAccessManager::createRequest(op, req, outgoingData);
        emit requestCreated(op, req, reply);
    } else {
        reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
        emit requestCreated(op, request, reply);
    }

    return reply;
}

