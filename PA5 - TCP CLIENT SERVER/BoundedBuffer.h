#ifndef _BOUNDEDBUFFER_H_
#define _BOUNDEDBUFFER_H_

#include <utility>
#include <queue>
#include <vector>
#include <mutex>
#include <cassert>
#include <cstring>
#include <condition_variable>

typedef char byte_t;
typedef std::condition_variable cv_t;

class BoundedBuffer {
private:
    // max number of items in the buffer
	int cap;

    /* The queue of items in the buffer
     * Note that each item a sequence of characters that is best represented by a vector<char> for 2 reasons:
	 *  1. An STL std::string cannot keep binary/non-printables
	 *  2. The other alternative is keeping a char* for the sequence and an integer length (b/c the items can be of variable length)
	 * While the second would work, it is clearly more tedious
     */
	std::queue<std::vector<char>> q;

	// add necessary synchronization variables and data structures 
	std::mutex mut;
	std::pair<cv_t,cv_t> cv;

public:

	size_t size();

	BoundedBuffer(int _cap);
	~BoundedBuffer();

	void push(char* msg, int size);
	int pop(char* msg, int size);

};

#endif