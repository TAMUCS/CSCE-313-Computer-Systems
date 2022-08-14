#include "RequestChannel.h"

using namespace std;


RequestChannel::RequestChannel (const string _name, const Side _side) : my_name(_name), my_side(_side) {
    // modify as needed
}

RequestChannel::~RequestChannel () {
    // modify as needed
}

std::string RequestChannel::name () { return my_name; }
