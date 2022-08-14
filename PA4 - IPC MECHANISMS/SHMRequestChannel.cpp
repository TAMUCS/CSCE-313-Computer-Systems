#include <sys/mman.h>

#include "SHMRequestChannel.h"

using namespace std;

#define ID(x,y) ("SHM_" + x + to_string(y))
#define cID(x,y) (x + "sem" + to_string(y)).c_str()

SHMQueue::SHMQueue (const string _name, int _length) : name(_name), length(_length) {
    int open = shm_open(_name.c_str(), O_RDWR | O_CREAT, 0666);
    err(open, "shm_open()"); 
    ftruncate(open, _length); 
    segment = (byte_t*)mmap(NULL, _length, PROT_READ | PROT_WRITE, MAP_SHARED, open, 0);
    send_done = sem_open(cID(name, 1), O_CREAT, 0666, 0);
    recv_done = sem_open(cID(name, 2), O_CREAT, 0666, 1);
}

SHMQueue::~SHMQueue () {
    munmap(segment, length);

    shm_unlink(name.c_str());

    sem_close(recv_done);
    sem_close(send_done);

    sem_unlink(cID(name, 1));
    sem_unlink(cID(name, 2));
}

int SHMQueue::shm_receive (void* msgbuf, int msgsize) {
    sem_wait(send_done);
    std::memcpy(msgbuf, segment, msgsize);
    sem_post(recv_done);
    return msgsize;
}

int SHMQueue::shm_send (void* msgbuf, int msgsize) {
    sem_wait(recv_done);
    std::memcpy(segment, msgbuf, msgsize);
    sem_post(send_done);
    return msgsize;
}

SHMRequestChannel::SHMRequestChannel(const std::string _name, const Side _side, const int m) : RequestChannel(_name, _side) {
    if (my_side == CLIENT_SIDE) {
        SHM1 = new SHMQueue(ID(my_name, 2), m);
        SHM2 = new SHMQueue(ID(my_name, 1), m);
	}
	else {
        SHM1 = new SHMQueue(ID(my_name, 1), m);
        SHM2 = new SHMQueue(ID(my_name, 2), m);
	}
}

SHMRequestChannel::~SHMRequestChannel() {
    delete SHM1;
    delete SHM2;
}

int SHMRequestChannel::cread (void* msgbuf, int msgsize) {
    return SHM1->shm_receive(msgbuf, msgsize);
}

int SHMRequestChannel::cwrite (void* msgbuf, int msgsize) {
    return SHM2->shm_send(msgbuf, msgsize);
}