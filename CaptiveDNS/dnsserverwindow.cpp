#include "dnsserverwindow.h"
#include "ui_dnsserverwindow.h"

#include <QDesktopServices>

/* YourFriendlyDNS - A really awesome multi-platform (lin,win,mac,android) local caching and proxying dns server!
Copyright (C) 2018  softwareengineer1 @ github.com/softwareengineer1
Support my work by sending me some Bitcoin or Bitcoin Cash in the value of what you valued one or more of my software projects,
so I can keep bringing you great free and open software and continue to do so for a long time!
I'm going entirely 100% free software this year in 2018 (and onwards I want to) :)
Everything I make will be released under a free software license! That's my promise!
If you want to contact me another way besides through github, insert your message into the blockchain with a BCH/BTC UTXO! ^_^
Thank you for your support!
BCH: bitcoincash:qzh3knl0xeyrzrxm5paenewsmkm8r4t76glzxmzpqs
BTC: 1279WngWQUTV56UcTvzVAnNdR3Z7qb6R8j
(These are the payment methods I currently accept,
if you want to support me via another cryptocurrency let me know and I'll probably start accepting that one too)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

DNSServerWindow::DNSServerWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::DNSServerWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<ListEntry>("ListEntry");
    qRegisterMetaType<std::vector<ListEntry>>("std::vector<ListEntry>");
    qRegisterMetaType<QHostAddress>("QHostAddress");

    settingspath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir d{settingspath};
    if(d.mkpath(d.absolutePath()))
        qDebug() << "CaptiveDNS settings storage location:" << settingspath;

    settingspath += QDir::separator();
    qDebug() << "CaptiveDNS settings location:" << settingspath;

    settings = new SettingsWindow();
    connect(settings, SIGNAL(settingsUpdated()), this, SLOT(settingsUpdated()));
    connect(settings, SIGNAL(setIPToFirstListening()), this, SLOT(setIPToFirstListening()));
    connect(settings, SIGNAL(autoCaptureCaptivePortals()), this, SLOT(autoCaptureCaptivePortals()));
    connect(settings, SIGNAL(iptablesUndoAndroid()), this, SLOT(iptablesUndoAndroid()));
    connect(settings->indexhtml, SIGNAL(htmlChanged(QString&)), this, SLOT(htmlChanged(QString&)));

    cacheviewer = new CacheViewer();
    connect(this, SIGNAL(displayCache(const std::vector<DNSInfo>&)), cacheviewer, SLOT(displayCache(const std::vector<DNSInfo>&)));

    preloadServerPorts();

    messagesThread = new MessagesThread();
    connect(messagesThread, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(messagesThread, SIGNAL(serversInitialized()), this, SLOT(serversInitialized()));
    connect(messagesThread, SIGNAL(androidInit()), this, SLOT(androidInit()));
    messagesThread->start();

    #ifdef Q_OS_ANDROID
    // ui->settingsButton->setIconSize(QSize(128,128));
    // ui->firstAddButton->setIconSize(QSize(64,64));
    // ui->secondAddButton->setIconSize(QSize(64,64));
    // ui->removeButton->setIconSize(QSize(64,64));
    #endif
    #ifdef Q_OS_MACOS
    // QFont font = ui->label->font();
    // font.setPointSize(11);
    // QList<QWidget*> widgets = this->findChildren<QWidget*>();
    // foreach (QWidget *widget, widgets)
    // {
    //     widget->setFont(font);
    // }
    #endif
}

DNSServerWindow::~DNSServerWindow()
{
    settingsSave();
    if(settings)
        delete settings;

    delete ui;
}

void DNSServerWindow::serversInitialized()
{
    server = AppData::get()->dnsServer;
    httpServer = AppData::get()->httpServer;
    connect(server, SIGNAL(queryRespondedTo(ListEntry)), this, SLOT(queryRespondedTo(ListEntry)));
    // connect(server->dnscrypt, &DNSCrypt::displayLastUsedProvider, this, &DNSServerWindow::displayLastUsedProvider);
    connect(settings, SIGNAL(clearDNSCache()), server, SLOT(clearDNSCache()));
    // connect(this, &DNSServerWindow::clearSources, settings->sourcerAndStampConverter, &providerSourcerStampConverter::clearSources);
    // connect(this, &DNSServerWindow::loadSource, settings->sourcerAndStampConverter, &providerSourcerStampConverter::loadSource);
    connect(cacheviewer, &CacheViewer::deleteEntriesFromCache, server, &SmallDNSServer::deleteEntriesFromCache);

    listeningIPsUpdate();
    settingsLoad();
    settingsUpdated();
    settings->setiptablesButtonEnabled(false);
}

void DNSServerWindow::displayLastUsedProvider(quint64 props, QString providerName, QHostAddress server, quint16 port)
{
    QString Props;
    if(props & 2) Props += "\nDoesn't log";
    if(props & 4) Props += "\nDoesn't filter";
    if(props & 1) Props += "\nSupports DNSSEC";
    // ui->encEnabled->setText(QString("ENCRYPTION ENABLED! :)\nProvider last used:\n%1\n%2:%3%4").arg(providerName).arg(server.toString()).arg(port).arg(Props));
}

void DNSServerWindow::androidInit()
{
    settings->setiptablesButtonEnabled();
}

void DNSServerWindow::htmlChanged(QString &html)
{
    if(httpServer)
        httpServer->setHTML(html);
}

void DNSServerWindow::listeningIPsUpdate()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    QVector<quint32> ipslist;
    QVector<Q_IPV6ADDR> ipv6slist;
    QString listeningips = "";
    if(list.size() > 0)
    {
        for(int i = 0; i < list.count(); i++)
        {
            if(!list[i].isLoopback())
            {
                // if(listeningips == "") listeningips = "Local DNS Address:\n";
                if(list[i].protocol() == QAbstractSocket::IPv4Protocol)
                {
                    ipslist.append(QHostAddress(list[i].toString()).toIPv4Address());
                    listeningips += list[i].toString() + "\n";
                }
                // No ipv6 in captivedns (yet?)

                // else if(list[i].protocol() == QAbstractSocket::IPv6Protocol)
                // {
                //     ipv6slist.append(QHostAddress(list[i].toString()).toIPv6Address());
                //     listeningips += "[" + QHostAddress(QHostAddress(list[i].toString()).toIPv6Address()).toString() + "], ";
                // }
            }
        }
        listeningips.truncate(listeningips.size()-1);
        // listeningips.append("\n\nEnter the above address into your device's DNS settings.");
    }

    if(ipslist.size() > 0)
    {
        server->listeningIPs = ipslist;
    }
    if(ipv6slist.size() > 0)
    {
        server->listeningIPv6s = ipv6slist;
    }

    if(listeningips == "")
    {
        listeningips = "No non-loopback listening network addresses detected...\
                        Check network connection!";
    }

    ui->listeningIPs->setText(listeningips);
}

void DNSServerWindow::setIPToFirstListening()
{
    if(server && settings)
    {
        listeningIPsUpdate();
        if(server->listeningIPs.size() > 0)
        {
            settings->setRespondingIP(QHostAddress(server->listeningIPs[0]).toString());
            server->ipToRespondWith = server->listeningIPs[0];
        }
        // if(server->listeningIPv6s.size() > 0)
        // {
        //     settings->setRespondingIPv6(QHostAddress(server->listeningIPv6s[0]).toString());
        //     server->ipv6ToRespondWith = server->listeningIPv6s[0];
        // }
    }
}

void DNSServerWindow::settingsUpdated()
{
    if(server && settings)
    {
        server->dnscryptEnabled = settings->getDNSCryptEnabled();
        // ui->encEnabled->setVisible(server->dnscryptEnabled);
        // server->dnscrypt->newKeyPerRequest = settings->getNewKeyPerRequestEnabled();
        server->blockmode_returnlocalhost = settings->blockmode_localhost;
        server->ipToRespondWith = QHostAddress(settings->getRespondingIP()).toIPv4Address();
        server->cachedMinutesValid = settings->getCachedMinutesValid();
        server->realdns = settings->returnRealDNSServers();
        server->dedicatedDNSCrypter = settings->returnDedicatedDNSCrypter();
        server->determineDoHDoTLSProviders();
        server->dnsTTL = settings->dnsTTL;
        server->autoTTL = settings->autoTTL;
    }
}

void DNSServerWindow::queryRespondedTo(ListEntry e)
{
    QString ip = e.ip ? QHostAddress(e.ip).toString() : "";

    // for(int i = 0; i < ui->dnsqueries->topLevelItemCount(); i++)
    // {
    //     if(ui->dnsqueries->topLevelItem(i)->text(1) == e.hostname)
    //     {
    //         ui->dnsqueries->topLevelItem(i)->setText(0, ip);
    //         return;
    //     }
    // }

    // ui->dnsqueries->addTopLevelItem(new QTreeWidgetItem(QStringList() << ip << e.hostname));
}

void DNSServerWindow::autoCaptureCaptivePortals()
{
    // the list that is used is in smalldnsserver.cpp around line 60

    // appendToBlacklist(ListEntry("ctest.cdn.nintendo.net"));
    // appendToBlacklist(ListEntry("conntest.nintendowifi.net"));
    // appendToBlacklist(ListEntry("detectportal.firefox.com"));
    // appendToBlacklist(ListEntry("connectivitycheck.gstatic.com"));
    // appendToBlacklist(ListEntry("connectivitycheck.android.com"));
    // appendToBlacklist(ListEntry("clients1.google.com"));
    // appendToBlacklist(ListEntry("clients3.google.com"));
    // appendToBlacklist(ListEntry("captive.apple.com"));
    refreshList();
}

void DNSServerWindow::iptablesUndoAndroid()
{
    #ifdef Q_OS_ANDROID
    AndroidSU_ServerOP *suOP = new AndroidSU_ServerOP(AndroidSU_ServerOP::opcode::iptablesRemove, AppData::get()->dnsServerPort, AppData::get()->httpServerPort);
    connect(suOP, SIGNAL(finished()), suOP, SLOT(deleteLater()));
    suOP->start();
    #endif
}

void DNSServerWindow::appendToBlacklist(ListEntry e)
{
    for(ListEntry &entry : server->blacklist)
    {
        if(entry.hostname == e.hostname)
            return;
    }
    server->blacklist.append(e);
}

void DNSServerWindow::on_firstAddButton_clicked()
{
    bool append = true;
    // ListEntry e(ui->hostnameEdit->text());
    // if(e.hostname.isEmpty()) return;
    // if(!ui->ipEdit->text().isEmpty())
    //     e.ip = QHostAddress(ui->ipEdit->text()).toIPv4Address();

    // ui->hostnameEdit->clear();
    // ui->ipEdit->clear();
    if(server->whitelistmode)
    {
        for(ListEntry &entry : server->whitelist)
        {
            // if(entry.hostname == e.hostname)
            // {
            //     // entry.ip = e.ip;
            //     append = false;
            // }
        }
        // if(append)
        //     server->whitelist.append(e);
    }
    else
    {
        for(ListEntry &entry : server->blacklist)
        {
            // if(entry.hostname == e.hostname)
            // {
            //     // entry.ip = e.ip;
            //     append = false;
            // }
        }
        // if(append)
        //     server->blacklist.append(e);
    }

    // if(append)
    //     ui->dnslist->addTopLevelItem(new QTreeWidgetItem(QStringList() << ui->ipEdit->text() << ui->hostnameEdit->text()));

    refreshList();
}

void DNSServerWindow::refreshList()
{
    // ui->dnslist->clear();
    // ui->dnslist->clear();
    // if(server->whitelistmode)
    // {
    //     for(ListEntry &e : server->whitelist)
    //     {
    //         if(e.ip == 0)
    //             ui->dnslist->addTopLevelItem(new QTreeWidgetItem(QStringList() << "" << e.hostname));
    //         else
    //             ui->dnslist->addTopLevelItem(new QTreeWidgetItem(QStringList() << QHostAddress(e.ip).toString() << e.hostname));
    //     }
    // }
    // else
    // {
    //     for(ListEntry &e : server->blacklist)
    //     {
    //         if(e.ip == 0)
    //             ui->dnslist->addTopLevelItem(new QTreeWidgetItem(QStringList() << "" << e.hostname));
    //         else
    //             ui->dnslist->addTopLevelItem(new QTreeWidgetItem(QStringList() << QHostAddress(e.ip).toString() << e.hostname));
    //     }
    // }
}

void DNSServerWindow::on_whitelistButton_clicked()
{
    server->whitelistmode = true;
    // ui->whitelistButton->setChecked(true);
    refreshList();
}

void DNSServerWindow::on_blacklistButton_clicked()
{
    server->whitelistmode = false;
    // ui->blacklistButton->setChecked(true);
    refreshList();
}

bool DNSServerWindow::settingsSave()
{
    return false; // not used in captivedns

    // QFile file(settingspath);
    // if(file.open(QFile::WriteOnly))
    // {
    //     QJsonObject json;
    //     json["version"] = "2.1";
    //     server->dnscryptEnabled = settings->getDNSCryptEnabled();
    //     json["dnscryptEnabled"] = server->dnscryptEnabled;
    //     // server->dnscrypt->newKeyPerRequest = settings->getNewKeyPerRequestEnabled();
    //     json["dedicatedDNSCrypter"] = server->dedicatedDNSCrypter;
    //     // json["newKeyPerRequest"] = server->dnscrypt->newKeyPerRequest;
    //     json["initialMode"] = server->initialMode;
    //     json["whitelistmode"] = server->whitelistmode;
    //     json["blockmode_returnlocalhost"] = server->blockmode_returnlocalhost;
    //     json["autoinjectip"] = settings->autoinject;
    //     server->ipToRespondWith = QHostAddress(settings->getRespondingIP()).toIPv4Address();
    //     json["ipToRespondWith"] = (int)server->ipToRespondWith;
    //     json["cachedMinutesValid"] = (int)server->cachedMinutesValid;
    //     json["dnsTTL"] = (int)server->dnsTTL;
    //     json["autoTTL"] = server->autoTTL;
    //     AppData::get()->dnsServerPort = settings->getDNSServerPort().toInt();
    //     json["dnsServerPort"] = AppData::get()->dnsServerPort;
    //     AppData::get()->httpServerPort = settings->getHTTPServerPort().toInt();
    //     json["httpServerPort"] = AppData::get()->httpServerPort;
    //     html = settings->indexhtml->getHTML();
    //     json["html"] = html;

    //     QJsonArray dnsarray;
    //     foreach(const QString dns, server->realdns)
    //     {
    //         dnsarray.append(dns);
    //     }
    //     json["real_dns_servers"] = dnsarray;

    //     QJsonArray sourcesarray;
    //     // foreach(const ProviderSource &s, settings->sourcerAndStampConverter->providerSources)
    //     // {
    //     //     QJsonObject subObject;
    //     //     subObject["url"] = s.url;
    //     //     if(s.hash.size() > 0)
    //     //         subObject["hash"] = s.hash.toHex().toStdString().c_str();
    //     //     subObject["lastUpdatedTime"] = s.lastUpdated.toString();
    //     //     sourcesarray.append(subObject);
    //     // }
    //     json["dnscrypt_provider_sources"] = sourcesarray;

    //     QJsonArray whitelistarray;
    //     foreach(const ListEntry w, server->whitelist)
    //     {
    //         QJsonObject subObject;
    //         subObject["hostname"] = w.hostname;
    //         if(w.ip != 0) //No sense wasting space in the json file for null values
    //             subObject["ip"] = (int)w.ip;
    //         whitelistarray.append(subObject);
    //     }
    //     json["whitelist"] = whitelistarray;

    //     QJsonArray blacklistarray;
    //     foreach(const ListEntry b, server->blacklist)
    //     {
    //         QJsonObject subObject;
    //         subObject["hostname"] = b.hostname;
    //         if(b.ip != 0)
    //             subObject["ip"] = (int)b.ip;
    //         blacklistarray.append(subObject);
    //     }
    //     json["blacklist"] = blacklistarray;

    //     QJsonDocument jsondoc(json);
    //     file.write(jsondoc.toJson());
    //     file.close();
    //     return true;
    // }

    // return false;
}

void DNSServerWindow::showInFolder(const QString& path)
{
    QFileInfo info(path);
#if defined(Q_OS_WIN)
    QStringList args;
    if (!info.isDir())
        args << "/select,";
    args << QDir::toNativeSeparators(path);
    if (QProcess::startDetached("explorer", args))
        return;
#elif defined(Q_OS_MAC)
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \"" + path + "\"";
    args << "-e";
    args << "end tell";
    args << "-e";
    args << "return";
    if (!QProcess::execute("/usr/bin/osascript", args))
        return;
#endif
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir()? path : info.path()));
}

void DNSServerWindow::on_editHTMLButton_clicked() {
    // this editor window kind-of works, but let's just open the containing folder in the system file browser instead
    // TODO: re-enable this later
    // settings->indexhtml->show();

    // open html folder
    showInFolder(httpServer->getIndexHTMLPath());
}

void DNSServerWindow::on_aboutApp_clicked() {
    // open about site
    auto url = "https://github.com/browsedns/captivedns";
    QDesktopServices::openUrl(QUrl(url));
}

void DNSServerWindow::preloadServerPorts()
{
    QFile file(settingspath);
    if(!file.open(QFile::ReadOnly))
        return;

    QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();

    if(json.contains("dnsServerPort") && json["dnsServerPort"].isDouble())
    {
        AppData::get()->dnsServerPort = json["dnsServerPort"].toInt();
        settings->setDNSServerPort(AppData::get()->dnsServerPort);
    }
    qDebug() << "Using dns server port:" << AppData::get()->dnsServerPort;

    if(json.contains("httpServerPort") && json["httpServerPort"].isDouble())
    {
        AppData::get()->httpServerPort = json["httpServerPort"].toInt();
        settings->setHTTPServerPort(AppData::get()->httpServerPort);
    }
    qDebug() << "Using http server port:" << AppData::get()->httpServerPort;

    file.close();
}

bool DNSServerWindow::settingsLoad()
{
    // manually set YFDNS settings (can't be customized by user)

    // blacklist only (allow all non-captive portal domains)
    server->whitelistmode = false;
    on_blacklistButton_clicked();
    
    // no settings, so no initial mode
    server->initialMode = false;

    // always return our own IP
    setIPToFirstListening();

    settings->autoinject = true;
    settings->setAutoInject(true);

    refreshList();

    // create the landing.html file if it doesn't exist
    httpServer->createLanding();

    return true; // no settings
}

void DNSServerWindow::on_initialMode_stateChanged(int arg1)
{
    if(arg1)
        server->initialMode = true;
    else
        server->initialMode = false;

    qDebug() << "initial mode:" << server->initialMode;
}

void DNSServerWindow::on_saveButton_clicked()
{
    // settingsSave();
}

void DNSServerWindow::on_removeButton_clicked()
{
    // auto selected = ui->dnslist->selectedItems();
    // for(QTreeWidgetItem *i : selected)
    // {
    //     if(server->whitelistmode)
    //     {
    //         for(int x = 0; x < server->whitelist.size(); x++)
    //         {
    //             if(i->text(1) == server->whitelist[x].hostname)
    //             {
    //                 qDebug() << "Removing from whitelist:" << i->text(1);
    //                 server->whitelist.remove(x);
    //                 break;
    //             }
    //         }
    //     }
    //     else
    //     {
    //         for(int x = 0; x < server->blacklist.size(); x++)
    //         {
    //             if(i->text(1) == server->blacklist[x].hostname)
    //             {
    //                  qDebug() << "Removing from blacklist:" << i->text(1);
    //                 server->blacklist.remove(x);
    //                 break;
    //             }
    //         }
    //     }
    // }
    // qDeleteAll(selected);
}

void DNSServerWindow::on_hostnameEdit_returnPressed()
{
    on_firstAddButton_clicked();
}

void DNSServerWindow::on_ipEdit_returnPressed()
{
    on_firstAddButton_clicked();
}

void DNSServerWindow::on_secondAddButton_clicked()
{
    bool alreadyAdded = false;
    // auto selected = ui->dnsqueries->selectedItems();
    // if(server->whitelistmode)
    // {   
    //     for(QTreeWidgetItem *i : selected)
    //     {
    //         for(ListEntry &e : server->whitelist)
    //         {
    //             if(e.hostname == i->text(1))
    //             {
    //                 alreadyAdded = true;
    //                 break;
    //             }
    //         }

    //         if(!alreadyAdded)
    //             server->whitelist.append(ListEntry(i->text(1)));
    //     }
    // }
    // else
    // {
    //     for(QTreeWidgetItem *i : selected)
    //     {
    //         for(ListEntry &e : server->blacklist)
    //         {
    //             if(e.hostname == i->text(1))
    //             {
    //                 alreadyAdded = true;
    //                 break;
    //             }
    //         }

    //         if(!alreadyAdded)
    //             server->blacklist.append(ListEntry(i->text(1)));
    //     }
    // }
    refreshList();
}
void DNSServerWindow::on_settingsButton_clicked()
{
    settings->show();
}

void DNSServerWindow::on_cacheViewButton_clicked()
{
    emit displayCache(server->cachedDNSResponses);
    cacheviewer->show();
}
