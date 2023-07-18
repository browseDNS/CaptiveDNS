#include "smallhttpserver.h"

#include <QDesktopServices>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QDnsLookup>
#include <QHostAddress>
#include <QHostInfo>
#include <QtEndian>
#include <QtCore>
#include <QProcess>

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

SmallHTTPServer::SmallHTTPServer(QObject *parent)
{
    Q_UNUSED(parent);
    html = "CaptiveDNS is working, but you shouldn't be seeing this message!";
    response_header=R"(HTTP/1.1 200 OK
Content-Type: %1
Content-Encoding: %2
Content-Length: %3
Accept-Ranges: %4
Date: %5
Connection: %6)";
    response_header += "\r\n\r\n";
    contentType = "text/html", encodingType = "identity", acceptRanges = "bytes", connection = "close";

    connect(this, &QTcpServer::newConnection, this, &SmallHTTPServer::returnIndexPage);
}

bool SmallHTTPServer::startServer(QHostAddress address, quint16 port)
{
    if(port == 0)
    {
        qDebug() << "HTTP server disabled in settings (set to port 0)";
        return false;
    }
    return listen(address, port);
}

void SmallHTTPServer::setHTML(QString html)
{
    this->html = html;
}

QString SmallHTTPServer::getIndexHTMLPath() {
    // TODO: copied from dnsserverwindow.cpp, refactor this
    auto settingspath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    settingspath += QDir::separator();
    return settingspath + "landing.html";
}

// we never cache the index page, and just always read it from disk
// this has worse performance, but it's easier to edit and reload
QString SmallHTTPServer::getHTMLContent()
{
    // html = settings->indexhtml->getHTML();

    // read the html from the landing page
    auto landingPath = getIndexHTMLPath();

    // create it, if it doesn't exist
    createLanding();

    if (QFile::exists(landingPath)) {
        QFile file(landingPath);
        if (file.open(QIODevice::ReadOnly)) {
            html = file.readAll();
            file.close();
        }
    }

    return html;
}

// creates the landing page, if it doesn't already exist
void SmallHTTPServer::createLanding()
{
    auto landingPath = getIndexHTMLPath();
    if (!QFile::exists(landingPath)) {
        auto defaultHtml = "\
<html><head>\n\
    <title>CaptiveDNS</title>\n\
    <style>\n\
        body {\n\
            font-family: sans-serif;\n\
            background-color: #eee;\n\
        }\n\
        h1 {\n\
            font-size: 1.5em;\n\
            background-color: #333;\n\
            color: #fff;\n\
            text-align: center;\n\
            padding: 0.5em;\n\
        }\n\
        p, li {\n\
            margin-left: 2em;\n\
            font-size: 1.2em;\n\
        }\n\
    </style>\n\
</head><body>\n\
    <h1>CaptiveDNS</h1>\n\
    <p>Welcome to the CaptiveDNS landing page!</p>\n\
    <p>Quick Links:</p>\n\
    <ul>\n\
        <li><a href=\"https://browsedns.net\">BrowseDNS</a></li>\n\
        <li><a href=\"https://dns.switchbru.com\">Switchbru Dashboard</a></li>\n\
        <li><a href=\"https://google.com\">Google</a></li>\n\
        <li><a href=\"https://duckduckgo.com\">DuckDuckGo</a></li>\n\
        <li><a href=\"https://startpage.com\">StartPage</a></li>\n\
    </ul>\n\
    <p>To customize this page, press \"Edit Landing Page\" in the CaptiveDNS app, and then open `landing.html` in a text editor.</p>\n\
    <p>For source code and license information, see <a href=\"https://github.com/browsedns/captivedns\">here</a>.</p>\n\
    <p><a href=\"javascript:window.location.reload();\">Reload Page</a></p>\n\
</body></html>";

        QFile indexhtml(landingPath);
        if(indexhtml.open(QFile::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&indexhtml);
            out << defaultHtml << Qt::endl;
        }

        // if the readme doesn't exist, create it
        // auto readmePath = settingspath + "README.txt";
        // if (!QFile::exists(readmePath)) {
        //     QFile readme(readmePath);
        //     if(readme.open(QFile::WriteOnly | QIODevice::Text))
        //     {
        //         QTextStream out(&readme);
        //         out << "This is the CaptiveDNS internal folder. You can customize the captive portal start page by editing the landing.html file in a text editor." << Qt::endl;
        //     }
        // }
    }
}

void SmallHTTPServer::returnIndexPage()
{
    // always reload the html content in case it changed
    getHTMLContent();

    QTcpSocket *socket = nextPendingConnection();
    connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);

    QString contentLength = QString("%1").arg(html.size());
    QString currentDateTime = QDateTime::currentDateTime().toString();
    QString response = response_header.arg(contentType).arg(encodingType).arg(contentLength).arg(acceptRanges).arg(currentDateTime).arg(connection);
    response += html;

    socket->write(response.toUtf8());
    socket->disconnectFromHost();
    //qDebug() << "[" << socket->socketDescriptor() << "] Wrote index page:" << response;
}
