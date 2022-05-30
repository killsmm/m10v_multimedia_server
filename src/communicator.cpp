#include "communicator.h"
#include <iostream>




Communicator::Communicator(std::string pub_url, std::string sub_url, std::string req_url) : zmqContext(), publisher(zmqContext, zmq::socket_type::pub), 
                                subscriber(zmqContext, zmq::socket_type::sub), cmd_server(zmqContext, zmq::socket_type::rep) {
    publisher.bind(pub_url);
    cmd_server.bind(req_url);
    subscriber.connect(sub_url);
}

Communicator::~Communicator() {
    
}



void Communicator::broadcast(std::string topic, std::string content) {
    std::string combo = topic + content;
    zmq::message_t msg(combo.c_str(), combo.size());
    publisher.send(msg);
}

bool Communicator::receiveSub(std::string topic, std::string &content) {
    zmq::message_t msg(128);
    memset(msg.data(), 0, 128);
    subscriber.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.length());
    if (subscriber.recv(&msg, ZMQ_DONTWAIT)){
        content = std::string(static_cast<const char *> (msg.data()), msg.size());
        return true;
    }
    return false;
}


bool Communicator::receiveCmd(RecvCallback cb) {
    zmq::message_t msg(128);
    memset(msg.data(), 0, 128);
    if (cmd_server.recv(&msg, ZMQ_DONTWAIT)){
        std::string cmd(static_cast<const char *> (msg.data()), msg.size());
        if(cb(cmd)){
            const char *response = "{\
                \"result\":\"success\"\
            }";
            cmd_server.send(response, strlen(response));
        }else{
            const char *response = "{\
                \"result\":\"failed\"\
            }";
            cmd_server.send(response, strlen(response));
        }
        return true;
    }
}
