#include "BoundedBuffer.h"

using namespace std;


BoundedBuffer::BoundedBuffer (int _cap) : cap(_cap) {}
BoundedBuffer::~BoundedBuffer () {}

void BoundedBuffer::push(byte_t* msg, int size) {
    byte_t* len(msg+size);
    std::vector<byte_t> msg_(msg, len);
    std::unique_lock<std::mutex> lck(mut);
    cv.second.wait(lck,[this]{return ((int)(this->size()))<cap;});
    q.push(msg_);
    lck.unlock();
    cv.first.notify_one();
}

int BoundedBuffer::pop(byte_t* msg, int size) {
    std::unique_lock<std::mutex> lck(mut);
    cv.first.wait(lck,[this]{return this->size()>=1;});
    std::vector<byte_t> popItem(q.front());
    std::size_t len(popItem.size());
    byte_t* dat(popItem.data());
    q.pop();
    lck.unlock();
    assert(((int)(popItem.size()))<=size);
    std::memcpy(msg, dat, len);
    cv.second.notify_one();
    return len;
}

size_t BoundedBuffer::size () { return q.size(); }