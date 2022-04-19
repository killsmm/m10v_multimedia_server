#ifndef _COMUNICATOR_H
#define _COMUNICATOR_H

#include <zmq.hpp>

class Communicator{
    public:
        Communicator(std::string pub_url, std::string sub_url);
        ~Communicator();
        void broadcast(std::string topic, std::string content);
        bool receive(std::string topic, std::string &str);
    private:
        zmq::context_t zmqContext;
        zmq::socket_t publisher;
        zmq::socket_t subscriber;
};

#endif 