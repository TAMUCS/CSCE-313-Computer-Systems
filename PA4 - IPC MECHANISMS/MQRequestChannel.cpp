#include "MQRequestChannel.h"
#include "common.h"

using namespace std;

#define ATTRMAXMSG 1
#define MQID(x,y) ("/MQ_" + x + to_string(y))
#define cMQID(x,y) ("/MQ_" + x + to_string(y)).c_str()

MQRequestChannel::MQRequestChannel(const std::string _name, const Side _side, const int m) : RequestChannel(_name, _side), name(_name), m(m) {
    if (my_side == CLIENT_SIDE) {
		rfd = open_MQ(MQID(my_name, 1), O_RDONLY | O_CREAT);
		wfd = open_MQ(MQID(my_name, 2), O_WRONLY | O_CREAT);
	}
	else {
		wfd = open_MQ(MQID(my_name, 1), O_WRONLY | O_CREAT);
		rfd = open_MQ(MQID(my_name, 2), O_RDONLY | O_CREAT);
	}
}

MQRequestChannel::~MQRequestChannel() {
    mq_close(rfd);
    mq_close(wfd);

    mq_unlink(cMQID(name, 1));
    mq_unlink(cMQID(name, 2));
}

mqd_t MQRequestChannel::open_MQ(std::string MQ_name, int _mode) {
    mq_attr attr{};
    attr.mq_maxmsg = ATTRMAXMSG; 
    attr.mq_msgsize = m;  
    return mq_open(MQ_name.c_str(), _mode, 0666, &attr);
}

int MQRequestChannel::cread (void* msgbuf, int msgsize) {
    return mq_receive(rfd, (byte_t*)msgbuf, _8KB, NULL); 
}

int MQRequestChannel::cwrite (void* msgbuf, int msgsize) {
    return mq_send(wfd, (byte_t*)msgbuf, msgsize, 0);
}