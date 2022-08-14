#include "FIFORequestChannel.h"

using namespace std;


FIFORequestChannel::FIFORequestChannel (const string _name, const Side _side) : RequestChannel(_name, _side) {
	fifo_name1 = "fifo_" + my_name + "1";
	fifo_name2 = "fifo_" + my_name + "2";

	if (my_side == CLIENT_SIDE) {
		rfd = open_fifo(fifo_name1, O_RDONLY);
		wfd = open_fifo(fifo_name2, O_WRONLY);
	}
	else {
		wfd = open_fifo(fifo_name1, O_WRONLY);
		rfd = open_fifo(fifo_name2, O_RDONLY);
	}
}

FIFORequestChannel::~FIFORequestChannel () {
	close(rfd);
	close(wfd);
	
	remove(fifo_name1.c_str());
	remove(fifo_name2.c_str());
}

int FIFORequestChannel::open_fifo (string _fifo_name, int _mode) {
	mkfifo(_fifo_name.c_str(), S_IRUSR | S_IWUSR);

	int fd = open(_fifo_name.c_str(), _mode);
	if (fd < 0) {
		EXITONERROR(_fifo_name);
	}
	return fd;
}

int FIFORequestChannel::cread (void* msgbuf, int msgsize) {
	return read(rfd, msgbuf, msgsize); 
}

int FIFORequestChannel::cwrite (void* msgbuf, int msgsize) {
	return write(wfd, msgbuf, msgsize);
}
