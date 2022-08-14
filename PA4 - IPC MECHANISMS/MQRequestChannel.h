#ifndef _MQREQUESTCHANNEL_H_
#define _MQREQUESTCHANNEL_H_

#include "RequestChannel.h"

#define _8KB 8192

class MQRequestChannel : public RequestChannel {
private:

    mqd_t wfd, rfd;
    std::string name;
    int m;

    mqd_t open_MQ(std::string _fifo_name, int _mode);
    
public:

    MQRequestChannel(const std::string _name, const Side _side, const int msgsize);
    ~MQRequestChannel();

    int cread (void* msgbuf, int msgsize);
	int cwrite (void* msgbuf, int msgsize);
};
    
#endif
