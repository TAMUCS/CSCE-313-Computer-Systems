#ifndef _FIFOREQUESTCHANNEL_H_
#define _FIFOREQUESTCHANNEL_H_

#include "RequestChannel.h"


class FIFORequestChannel : public RequestChannel {
private:
	int wfd;
    int rfd;

    std::string fifo_name1;
    std::string fifo_name2;
	
    int open_fifo (std::string _fifo_name, int _mode);

public:
	FIFORequestChannel (const std::string _name, const Side _side);
	~FIFORequestChannel ();

	int cread (void* msgbuf, int msgsize);
	int cwrite (void* msgbuf, int msgsize);
};

#endif
