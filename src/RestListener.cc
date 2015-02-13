/*
 * Copyright 2015 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "RestListener.hh"

REGISTER_APPLICATION(RestListener, {"controller", ""})

void Listeners::add_listener(std::string name, Rest *r)
{
    listeners[name] = r;
    LOG(INFO) << "REST listener (" <<  name << ") was added";
}

void Listeners::remove_listener(std::string name)
{
    //TODO: add mutex
    listeners.erase(name);
    LOG(INFO) << "REST listener (" <<  name << ") was removed";
}

std::string Listeners::formListenersJSON()
{
    JSONparser res;
    for (auto& it : listeners) {
        JSONparser l;
        l.addValue("name", it.second->display_name);
        l.addValue("page", it.second->webpage == "none" ? "#" : it.second->webpage);
        res.addValue("applications", l);
    }
    return res.get();
}

std::unordered_map<std::string, Rest*> Listeners::getListeners()
{
    return listeners;
}

void RestListener::newListener(std::string name, Rest* rest)
{
    l->add_listener(name, rest);
}

void RestListener::init(Loader *loader, const Config &config)
{
    tcpServer = new QTcpServer(this);
    l = new Listeners;

    auto app_config = config_cd(config, "rest-listener");
    listen_port = config_get(app_config, "port", 8000);
    web_dir = "./build/web";

    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(new_request()));
    tcpServer->listen(QHostAddress::Any, listen_port);
}

QString RestListener::proccessRequest(std::vector<std::string> params)
{
    std::string reply;
    if (l->getListeners().count(params[1])) {
        reply = l->getListeners()[params[1]]->handle(params);
    }
    else {
        reply = "{ \"error\": \"Not registered REST-application: " + params[1] + "\" }";
    }

    QString res(reply.c_str());
    return res;
}

void RestListener::new_request()
{
    QTcpSocket* clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(slotReadClient()));
    connect(clientSocket, SIGNAL(disconnected()), clientSocket, SLOT(deleteLater()));
}

void RestListener::slotReadClient()
{
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    std::vector<std::string> params;

    QString str = clientSocket->readAll();
    QStringList lines = str.split("\n");
    Req req = getHTTPheader(lines[0]);

    if (req.page == "/") {
        req.page = "start.html";
    }

    params.push_back(req.method.toStdString());
    if (!parsingGET(req.page, &params)) {       //api or apps or timeout
        sendFile(clientSocket, req.page);
        return;
    }

    if (req.page.contains("/apps")) {
        appsHandler(clientSocket, req);
        return;
    }

    if (req.page.contains("/timeout/")) {
        timeoutHandler(clientSocket, req, params);
        return;
    }

    if (req.page.contains("/wm/")) {
        translateRequest(&params);
    }

    // HTTP /api/...
    if (req.method == "POST" || req.method == "PUT") {
        parsingPOST(req, str.split("\r\n\r\n")[1], &params);
    }

    QString msg = proccessRequest(params);
    sendMsg(clientSocket, msg);
}

Req RestListener::getHTTPheader(QString head)
{
    Req req;
    QStringList list = head.split(" ");
    req.method = list[0];
    req.page = list[1];
    req.http_ver = list[2];
    return req;
}

bool RestListener::parsingGET(QString page, std::vector<std::string> *params)
{
    bool is_rest = false;
    if (page.contains("/api/"))
        is_rest = true;
    else if (page.contains("/timeout/"))
        is_rest = true;
    else if (page.contains("/apps"))
        is_rest = true;
    else if (page.contains("/wm/"))
        is_rest = true;

    if (is_rest) {
        QStringList list = page.split("/");
        for (auto it : list.mid(2))
            params->push_back(it.toStdString());
    }
    return is_rest;
}

bool RestListener::parsingPOST(Req req, QString body, std::vector<std::string> *params)
{
    QVector<QString> body_params;
    QString filepath(web_dir.c_str());
    filepath.append(PROTO_FILE);
    QFile file(filepath);
    QString line;

    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        line = stream.readLine();
        QStringList list = line.split(" ");
        if (list[0] != req.method)
            continue;
        if (list[1] != req.page)
            continue;
        for (size_t i = 2; i < list.size(); i++)
            body_params.push_back(list[i]);
        break;
    }
    file.close();

    JSONparser body_json;
    body_json.fill(body.toStdString());
    for (auto it : body_params) {
        std::string s = body_json.getValue(it.toStdString());
        if (s != "JSON::NON_VALUE")
            params->push_back(s);
        else {
            LOG(WARNING) << "incorrect request: " << body_json.get();
            return false;
        }
    }
    return true;
}

void RestListener::sendFile(QTcpSocket* sock, QString filename)
{
    QString filepath(web_dir.c_str());
    filepath.append(filename);
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        QByteArray reply("HTTP/1.1 404 Not found\r\n");
        sock->write(reply);
        sock->flush();
        sock->close();
        return;
    }

    QByteArray reply("HTTP/1.1 200 Ok\r\n");
    if (filename.contains(".css"))
        reply.append("Content-Type: text/css; charset=utf-8\r\n");
    else if (filename.contains(".png")) {
        reply.append("Content-Type: image/png\r\n");
    }
    else if (filename.contains(".js"))
        reply.append("Content-Type: application/x-javascript\r\n");
    else
        reply.append("Content-Type: text/html; charset=utf-8\r\n");

    QString size("Content-Length: ");
    size.append(QString::number(file.size()));
    reply.append(size);
    reply.append("\r\n\r\n");

    reply.append(file.readAll());
    sock->write(reply);
    sock->flush();
}

void RestListener::sendMsg(QTcpSocket* sock, QString msg)
{
    QByteArray header;
    header  = "HTTP/1.1 200 Ok\r\n";
    header += "Content-Type: text/html; charset=utf-8\r\n";
    header += "Content-Length: ";
    header += QString::number(msg.size());
    header += "\r\n";
    header += "\r\n";
    header += msg;
    sock->write(header);
    sock->flush();
}

void RestListener::translateRequest(std::vector<std::string> *params)
{
    if ((*params)[1] == "core" &&
        (*params)[2] == "controller" ) {

        if ((*params)[3] == "switches" &&
            (*params)[4] == "json"  ) {
            params->insert(params->begin() + 1, "switch-manager");
        }

        else if ((*params)[3] == "summary" &&
                 (*params)[4] == "json") {
            params->insert(params->begin() + 1, "controller-manager");
        }
        return;
    }

    if ((*params)[1] == "core") {

        if ((*params)[2] == "memory"  ||
            (*params)[2] == "health"  ||
            (*params)[2] == "system") {
            params->insert(params->begin() + 1, "controller-manager");
        }
        return;
    }

    if ((*params)[1] == "topology" &&
             (*params)[2] == "links" ) {
        (*params)[2] = "f_links";
        return;
    }

    if ((*params)[1] == "device") {
        (*params)[1] = "host-manager";
        params->push_back("f_hosts");
        return;
    }
}

void RestListener::appsHandler(QTcpSocket *sock, Req req)
{
    std::string res;
    if (req.method != "GET") {
        res = "{ \"error\": \"Incorrect HTTP method\" }";
    }
    else {
        res = l->formListenersJSON();
    }
    QString msg(res.c_str());
    sendMsg(sock, msg);
}

void RestListener::timeoutHandler(QTcpSocket* sock, Req req, std::vector<std::string> params)
{
    if (req.method != "GET") {
        QString msg = "{ \"error\": \"Incorrect HTTP method\" }";
        sendMsg(sock, msg);
        return;
    }

    JSONparser res;
    QString apps(params[1].c_str());
    res.addKey("timeout");
    auto listeners = l->getListeners();

    QStringList list = apps.split("&");
    for (auto it : list) {
        std::string app = it.toStdString();
        if (listeners.count(app) == 0) {
            LOG(ERROR) << "Listener (" << app << ") does not found!";
            continue;
        }
        if (!listeners[app]->hasEventModel()) {
            LOG(ERROR) << "Listener (" << app << ") does not use event model!";
            continue;
        }
        uint32_t last_event = atoi(params[2].c_str());
        res.addValue("timeout", listeners[app]->em->timeout(app, last_event));
    }
    QString msg(res.get().c_str());
    sendMsg(sock, msg);
}

