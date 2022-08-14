#ifndef _REQUESTCHANNEL_H_
#define _REQUESTCHANNEL_H_

#include "common.h"

inline void err(int x, std::string y) {if (x<0) throw Oopsie_Daisies(y);}

class RequestChannel {
public:
	enum Side {SERVER_SIDE, CLIENT_SIDE};

protected:
    std::string my_name;
    Side my_side;

public:
    RequestChannel (const std::string _name, const Side _side);
    virtual ~RequestChannel ();

    virtual int cread (void* msgbuf, int msgsize) = 0;
    /* Blocking read; returns the number of bytes read.
        If the read fails, it returns -1. */
    virtual int cwrite (void* msgbuf, int msgsize) = 0;
    /* Write the data to the channel. The function returns
        the number of bytes written, or -1 when it fails */
    std::string name ();
};

#endif
