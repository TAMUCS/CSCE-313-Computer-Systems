#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdexcept>
#include <list>

#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "TCPRequestChannel.h"

// ecgno to use for datamsgs
#define ECGNO 1
//to == t @ 0
#define t0 0.0
// You know what this is for
auto q((byte_t*)QUIT_MSG);
// Const termination std::pair for histogram
std::pair<int, double> STOP(std::pair<int, double>(-1, -1.02));

using namespace std;

struct Hoon_Cranked : std::logic_error {
  Hoon_Cranked(std::string e, std::string d=" Just Cranked a Hoon") 
    : std::logic_error(e+d) {}
};

void patient_thread_function (const int p, const int n, BoundedBuffer* reqB) {
    datamsg* dm(new datamsg(p, t0, ECGNO));
    for(int i(0); i < n; i++) {
        reqB->push((byte_t*)dm, sizeof(datamsg));
        ++(*dm);
    }
    delete dm;
}

void file_thread_function (const int m, const std::string f, TCPRequestChannel* c, BoundedBuffer* reqB) {
    __int64_t flen, bytes(1), offset_(0), m_((__int64_t)m);
    std::size_t soFm(sizeof(filemsg) + f.size() + 1);
    filemsg* fTrans(new filemsg(offset_, bytes));
    filemsg* fm(new filemsg(0,0));
    byte_t* buf(new byte_t[soFm]);
    
    std::memcpy(buf, fm, sizeof(filemsg));
	std::strcpy(buf+sizeof(filemsg), f.c_str());
    c->cwrite(buf, soFm);
    c->cread(&flen, sizeof(__int64_t));
    FILE* rf(fopen(("received/"+f).c_str(), "wb"));
    std::fseek(rf, flen, SEEK_SET);
    std::fclose(rf);

    __int64_t remainder(flen);
    while (remainder > 0) {
        bytes = std::min(m_, remainder);
        fTrans->length = bytes;
        fTrans->offset = offset_;
        std::memcpy(buf, fTrans, sizeof(filemsg));
        reqB->push(buf,soFm);
        offset_ += bytes;
        remainder -= bytes;
    }

    delete fTrans;
    delete[] buf;
    delete fm;

}

void worker_thread_function (const int m, TCPRequestChannel* c, BoundedBuffer* reqB, BoundedBuffer* rspB, const std::string f="") {
    byte_t* buf(new byte_t[m]);
    byte_t* rbuf(new byte_t[m]);
    double reply(0);

    for(;;) {

        reqB->pop(buf,m);
        auto msg(*((MESSAGE_TYPE*)buf));

        if (msg == DATA_MSG) {

            datamsg* dm((datamsg*)buf);
            c->cwrite(buf, sizeof(datamsg));
            c->cread(&reply, sizeof(double));
            auto send(std::make_pair(dm->person, reply));
            rspB->push((byte_t*)&send, sizeof(std::pair<int,double>));
                
        } else if(msg == FILE_MSG) { 

            filemsg* fm((filemsg*)buf);
            std::string fileDir("received/"+f);
            c->cwrite(buf,(sizeof(filemsg)+f.size()+1));
            c->cread(rbuf, m);
            FILE* file(std::fopen(fileDir.c_str(), "r+")); 
            std::fseek(file, fm->offset, SEEK_SET);
            std::fwrite(rbuf, sizeof(byte_t), fm->length, file);
            std::fclose(file);

        } else if (msg == QUIT_MSG) {
            break;
        } else { 
            throw Hoon_Cranked("worker_thread_function()"); 
        }
    }

    delete[] buf;
    delete[] rbuf;

}

void histogram_thread_function (const int m, HistogramCollection* hc, BoundedBuffer* rspB) {
    byte_t* buf(new byte_t[m]);

    for(;;) {
        rspB->pop(buf,m);
        auto data((std::pair<int,double>*)buf);
        if (*data == STOP) { break; }
        hc->update(data->first, data->second);
    }

    delete[] buf;

}

int main (int argc, char* argv[]) {
    int n(1000);	// default number of requests per "patient"
    int p(10);		// number of patients [1,15]
    int w(100);	// default number of worker threads
	int h(20);		// default number of histogram threads
    int b(20);		// default capacity of the request buffer (should be changed)
	int m(MAX_MESSAGE);	// default capacity of the message buffer
	std::string f(""), a("127.0.0.1"), r("8080");	// name of file to be transferred; IP; port no
    
    // Read Console Args
    int opt;
	while ((opt = getopt(argc, argv, "n:p:w:h:b:m:f:a:r:")) != -1) {
		switch (opt) {
			case 'n':
				n = atoi(optarg);
                break;
			case 'p':
				p = atoi(optarg);
                break;
			case 'w':
				w = atoi(optarg);
                break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
                break;
			case 'm':
				m = atoi(optarg);
                break;
			case 'f':
				f = optarg;
                break;
			case 'a':
				a = optarg;
                break;
			case 'r':
				r = optarg;
                break;
		}
	}
    
	// initialize overhead (including the control channel)
    BoundedBuffer* request_buffer(new BoundedBuffer(b));
    BoundedBuffer* response_buffer(new BoundedBuffer(b));
	HistogramCollection* hc(new HistogramCollection);

    // New Channel Resources
    std::vector<TCPRequestChannel*> channels(w);

    // Thread Vectors
    std::vector<std::thread*> pThreads(p);
    std::vector<std::thread*> wThreads(w);
    std::vector<std::thread*> hThreads(h);

    // File Thread & Accompanying Channel
    std::thread* fThread;
    TCPRequestChannel* fChan; 

    // making histograms and adding to collection
    for (int i = 0; i < p; i++) {
        Histogram* h(new Histogram(10, -2.0, 2.0));
        hc->add(h);
    }

	// record start time
    struct timeval start, end;
    gettimeofday(&start, 0);
 
    if (f.empty()) {
        // Init p Patient Threads
        for (int i(0); i < p; i++) {
            pThreads.at(i) = new std::thread(patient_thread_function, i+1, n, request_buffer);
        }

        // Init h Histogram Threads
        for (int i(0); i < h; i++) {
            hThreads.at(i) = new std::thread(histogram_thread_function, m, hc, response_buffer); 
        }

    } else {
        
        // Init File Thread
        fChan = new TCPRequestChannel(a, r);
        fThread = new std::thread(file_thread_function, m, f, fChan, request_buffer);

    }

    // Init w Channels & Worker Threads using w^th channel from vector
    for (int i(0); i < w; i++) {
        channels.at(i) = new TCPRequestChannel(a, r);
        wThreads.at(i) = new std::thread(worker_thread_function, m, channels.at(i), request_buffer, response_buffer, f);
    }

    if (f.empty()) { 

        // Patient Threads
        for(int i(0); i < p; i++) { pThreads.at(i)->join(); }

        // Push Stop Flags, Then Join Worker Threads
        for(int i(0); i < w; i++) { request_buffer->push((byte_t*)&q, sizeof(MESSAGE_TYPE)); }
        for(int i(0); i < w; i++) { wThreads.at(i)->join(); }

        // Push Stop Flags, Then Join Histogram Threads
        for(int i(0); i < h; i++) { response_buffer->push((byte_t*)&STOP, sizeof(std::pair<int,double>)); }
        for(int i(0); i < h; i++) { hThreads.at(i)->join(); }

    } else {

        // File Threads
        fThread->join();

        // Push Stop Flags, Then Join Worker Threads
        for(int i(0); i < w; i++) { request_buffer->push((byte_t*)&q, sizeof(MESSAGE_TYPE)); }
        for(int i(0); i < w; i++) { wThreads.at(i)->join(); }
    }

	// record end time
    gettimeofday(&end, 0);

    // Print the results
	if (f == "") { hc->print(); }
    int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
    int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
    std::cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << std::endl;

    // Quit & Dealloc w channels
    for(auto i : channels) {
        i->cwrite((byte_t*)&q, sizeof(MESSAGE_TYPE));
        delete i;
    }
    channels.clear();

    //Dealloc all threads p,w,h
    for(auto i : pThreads) { delete i; }
    for(auto i : hThreads) { delete i; }
    for(auto i : wThreads) { delete i; }

    // Quit & Dealloc all file related objects
    if (!f.empty()){ 
        fChan->cwrite((byte_t*)&q, sizeof(MESSAGE_TYPE));
        delete fChan;
        delete fThread; 
    }

    // Dealloc BoundedBuffers & Histogram Collection
    delete request_buffer;
    delete response_buffer;
    delete hc;
    
	// wait for server to exit
	wait(nullptr);
}