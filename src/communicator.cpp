#include "communicator.h"
#include <iostream>




Communicator::Communicator(std::string pub_url, std::string gps_url, std::string work_status_url, std::string req_url) : zmqContext(), publisher(zmqContext, zmq::socket_type::pub), 
                                gpsDataSubscriber(zmqContext, zmq::socket_type::sub), cmd_server(zmqContext, zmq::socket_type::rep), 
                                workStatusSubscriber(zmqContext, zmq::socket_type::sub) {
    publisher.bind(pub_url);
    cmd_server.bind(req_url);
    gpsDataSubscriber.connect(gps_url);
    workStatusSubscriber.connect(work_status_url);
}



Communicator::~Communicator() {
    
}



void Communicator::broadcast(std::string topic, std::string content) {
    std::string combo = topic + content;
    zmq::message_t msg(combo.c_str(), combo.size());
    publisher.send(msg);
}

bool Communicator::receiveSub(std::string &content) {
    zmq::message_t msg(1024);
    memset(msg.data(), 0, 128);
    gpsDataSubscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    if (gpsDataSubscriber.recv(&msg, ZMQ_DONTWAIT)){
        content = std::string(static_cast<const char *> (msg.data()), msg.size());
        return true;
    }
    workStatusSubscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    if (workStatusSubscriber.recv(&msg, ZMQ_DONTWAIT)){
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
