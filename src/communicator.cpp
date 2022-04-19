#include "communicator.h"


Communicator::Communicator(std::string pub_url, std::string sub_url) : zmqContext(), publisher(zmqContext, zmq::socket_type::pub), 
                                subscriber(zmqContext, zmq::socket_type::sub) {
    publisher.bind(pub_url);
    subscriber.connect(sub_url);
}

Communicator::~Communicator() {
    
}



void Communicator::broadcast(std::string topic, std::string content) {
    std::string combo = topic + content;
    zmq::message_t msg(combo.c_str(), combo.size());
    publisher.send(msg);
}

bool Communicator::receive(std::string topic, std::string &content) {
    zmq::message_t msg(128);
    memset(msg.data(), 0, 128);
    subscriber.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.length());
    if (subscriber.recv(&msg, ZMQ_DONTWAIT)){
        content = std::string(static_cast<const char *> (msg.data()), msg.size());
        return true;
    }
    return false;
}

