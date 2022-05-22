#ifndef _COMUNICATOR_H
#define _COMUNICATOR_H

#include <zmq.hpp>

typedef bool (*RecvCallback)(std::string);

class Communicator{
    public:
        Communicator(std::string pub_url, std::string sub_url, std::string req_url);
        ~Communicator();
        void broadcast(std::string topic, std::string content);
        bool receiveSub(std::string topic, std::string &str);
        bool receiveCmd(RecvCallback cb);
    private:
        zmq::context_t zmqContext;
        zmq::socket_t publisher;
        zmq::socket_t subscriber;
        zmq::socket_t cmd_server;
         
};

#endif 