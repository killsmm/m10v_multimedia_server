#ifndef _COMUNICATOR_H
#define _COMUNICATOR_H

#include <zmq.hpp>

static std::string PUBLISH_URL = "tcp://*:8101";

static std::string GPS_URL = "ipc:///tmp/gps";
// static std::string GPS_URL = "tcp://192.168.137.7:8890";


static std::string STATUS_URL = "tcp://127.0.0.1:8100";


static std::string REP_URL = "tcp://*:8102";

typedef bool (*RecvCallback)(std::string);

class Communicator{
    public:
        Communicator(std::string pub_url = PUBLISH_URL, std::string gps_url = GPS_URL, 
                    std::string work_status_url = STATUS_URL, std::string req_url = REP_URL);
        ~Communicator();
        void broadcast(std::string topic, std::string content);
        bool receiveSub(std::string &str);
        bool receiveCmd(RecvCallback cb);

    private:
        zmq::context_t zmqContext;
        zmq::socket_t publisher;
        zmq::socket_t gpsDataSubscriber;
        zmq::socket_t cmd_server;
        zmq::socket_t workStatusSubscriber;
};

#endif 