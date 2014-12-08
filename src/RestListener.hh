#pragma once

#include <QtNetwork>
#include <QTcpSocket>
#include <unordered_map>

#include <JsonParser.hh>
#include <Rest.hh>
#include <Application.hh>
#include <Controller.hh>
#include <Loader.hh>

#define PROTO_FILE "proto"
#define DEBUG 1

/**
 * Parsed first line of HTT request
 */
struct Req {
    /// HTTP-method
    QString method;

    /// request (or webpage)
    QString page;

    /// HTTP version (1.0 or 1.1)
    QString http_ver;
};

class Listeners : public QObject {
    Q_OBJECT
    std::unordered_map<std::string, Rest*> listeners;
private slots:
    /**
     * Adding new REST listener
     * Slot for Controller's signal: &Controller::newListener
     */
    void add_listener(std::string name, Rest* r);

    /**
     * Removing REST listener
     */
    void remove_listener(std::string name);
private:

    /**
     * Returns the list of the REST listeners in JSON-string format
     */
    std::string formListenersJSON();

    /**
     * Listener's getter
     */
    std::unordered_map<std::string, Rest*> getListeners();

    friend class RestListener;
};

/**
 * This application creates TCP server and listen some port (8000, by default).
 * It handles all HTTP requests including webpages and REST
 * If REST-request arrived, it parses request and send parameters to requested REST application.
 * When this application returned reply, RestListener send answer to client.
 */
class RestListener: public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(RestListener, "rest-listener")
public:
    void init(Loader* loader, const Config& config) override;

    /**
     * New REST listener in the controller.
     * Should be emitted in startUp() method in the application.
     * @param name Called name of the REST.
     * @param l Instance of the REST class.
     */
    void newListener(std::string name, Rest* rest);
private:
    QTcpServer* tcpServer;
    Listeners* l;
    int listen_port;
    std::string web_dir;

    /**
     * Parsing HTTP header
     */
    Req getHTTPheader(QString head);

    /**
     * Parsing GET request
     */
    bool parsingGET(QString page, std::vector<std::string> *params);

    /**
     * Parsing POST request
     */
    bool parsingPOST(Req req, QString body, std::vector<std::string> *params);

    /**
     * Sending parsed request to the some REST-application and returning application's reply
     */
    QString proccessRequest(std::vector<std::string> params);

    /**
     * Sending requested webpage (filename) to the client
     */
    void sendFile(QTcpSocket* sock, QString filename);

    /**
     * Sending the REST reply to the client
     */
    void sendMsg(QTcpSocket* sock, QString msg);

    /**
     * Translating request to Floodlight format
     */
    void translateRequest(std::vector<std::string>* params);

    /**
     * Handling "/apps/" request.
     * Sending the list of applications to the client
     */
    void appsHandler(QTcpSocket* sock, Req req);

    /**
     * Handling "/timeout/..." request
     * Sedning to the client occured events in selected applications
     */
    void timeoutHandler(QTcpSocket* sock, Req req, std::vector<std::string> params);
private slots:

    /**
     * New TCP request
     */
    void new_request();

    /**
     * Called when client's socket ready to read information
     */
    void slotReadClient();
};
