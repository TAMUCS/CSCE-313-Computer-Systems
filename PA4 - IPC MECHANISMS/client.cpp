#include "common.h"
#include <sys/wait.h>
#include <sys/time.h>

#include "RequestChannel.h"
#include "FIFORequestChannel.h"
#include "MQRequestChannel.h"
#include "SHMRequestChannel.h"

using namespace std;

#define t0 0.0
#define ECG1 1
#define ECG2 2
#define ONLY_P (p_ && !(t_||e_))
#define PTE (p_ && t_ && e_)

#define Print1DP(w,x,y,z) std::cout << "For person " << w << ", at time " << x 				   \
									<< ", the value of ecg " << y << " is " << z << std::endl  \

#define CSVline(x,y,z) x << "," << y << "," << z << std::endl									

typedef MESSAGE_TYPE msg_t;

/* [Single Data Point Request] reply by reference */
void dmRequest(RequestChannel* ch, byte_t* buf, double& reply) {
	ch->cwrite(buf, sizeof(datamsg)); 
	ch->cread(&reply, sizeof(double));
}

/* Client Side Channel Type */
RequestChannel* chanT(const int m, const std::string i, byte_t* buf=((byte_t*)"control")) {
	if (i == "f") { return (new FIFORequestChannel(buf, RequestChannel::CLIENT_SIDE)); }
	if (i == "q") { return (new MQRequestChannel(buf, RequestChannel::CLIENT_SIDE, m)); }
	if (i == "s") { return (new SHMRequestChannel(buf, RequestChannel::CLIENT_SIDE, m)); }
	return nullptr;
}

/* Single Client-Side New Channel */
RequestChannel* newChan(RequestChannel* chan, byte_t* buf, const int m, const std::string i) {
	msg_t ncm(NEWCHANNEL_MSG);
	chan->cwrite(&ncm, sizeof(msg_t));
	chan->cread(buf,m);
	return chanT(m,i,buf); 
}

/* Quits & Deallocs single Channel */
void delChan(RequestChannel* ch) {
	msg_t q(QUIT_MSG);
	ch->cwrite(&q, sizeof(msg_t));
	delete ch;
}

/* [Data Points -> CSV Request] */
void CSVTrans(RequestChannel* ch, const int m, const int p, const int n=1) {
	auto dm(std::make_pair(new datamsg(p,t0,ECG1), new datamsg(p,t0,ECG2)));
	auto buf(std::make_pair(new byte_t[m], new byte_t[m]));
	auto reply(std::make_pair(double(0.0), double(0.0)));
	std::ofstream dm1k("received/x" + to_string(n) + ".csv");

	while(dm.first->seconds < 4) {
		std::memcpy(buf.first, dm.first, sizeof(datamsg));
		dmRequest(ch, buf.first, reply.first);
		std::memcpy(buf.second, dm.second, sizeof(datamsg));
		dmRequest(ch, buf.second, reply.second);
		dm1k << CSVline(dm.first->seconds,reply.first,reply.second);
		++(*dm.first);
		++(*dm.second);
	}

	dm1k.close();
	delete dm.first;
	delete dm.second;
	delete[] buf.first;
	delete[] buf.second;
}

/* [File Transfer] */
void fileTrans(std::vector<RequestChannel*> ch, const int m, std::string fName, RequestChannel* ctrl) {
	__int64_t bytes(1), offset_(0), r((__int64_t)m), s(0);
	filemsg *fTrans(new filemsg(offset_, bytes)), *fm(new filemsg(0,0));
	std::size_t len(sizeof(filemsg)+fName.size()+1);
	byte_t* rBuf(new byte_t[m]), *wBuf(new byte_t[len]);

	std::memcpy(wBuf, fm, sizeof(filemsg));
	std::strcpy(wBuf+sizeof(filemsg),fName.c_str());
	FILE* f(fopen(("received/"+fName).c_str(), "wb"));
	ctrl->cwrite(wBuf, len);  
	ctrl->cread(&s, sizeof(__int64_t));

	if (ch.size() > 1) { r = (__int64_t)ceil(s/m); } 

	while (s > 0) {
		for (auto i : ch) {
			bytes = std::min(r, s); 
			fTrans->offset = offset_;
			fTrans->length = bytes;
			std::memcpy(wBuf, fTrans, sizeof(filemsg));
			i->cwrite(wBuf, len);
			i->cread(rBuf, bytes);
			std::fwrite(rBuf, sizeof(byte_t), bytes, f);
			s -= bytes;
			offset_ += bytes; 
			if (s <= 0) { break; }
		}
	}

	std::fclose(f);
	delete fTrans; 
	delete[] rBuf;
	delete[] wBuf;
	delete fm;
}

int main (int argc, char *argv[]) {
	bool p_(0), t_(0), e_(0), f_(0), m_(0), c_(0), i_(0); 
	int opt, p(1), e(1), c(0), m(MAX_MESSAGE), stat;
	std::string filename(""), mM(""), i("f"); 
	std::vector<std::string> argsV;
	double t(1);

	while ((opt = getopt(argc, argv, "p:t:e:f:m:i:c:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi(optarg);
				p_ = 1;
				break;
			case 't':
				t = atof(optarg);
				t_ = 1;
				break;
			case 'e':
				e = atoi(optarg);
				e_ = 1;
				break;
			case 'f':
				filename = optarg;
				f_ = 1;
				break;
			case 'm':
				m = atoi(optarg);
				mM = optarg;
				m_ = 1;
				break;
			case 'c': 
				c = atoi(optarg);
				c_ = 1; 
				break;
			case 'i': 
				i = optarg;
				i_ = 1;
				break;
			default: 
				throw Oopsie_Daisies("Client: getopt()");
				break;
		}
	}

	pid_t ID = fork();
	if (ID == 0) { /* [CHILD] SERVER */

		if (m_) { 
			if (i_) { argsV = {"./server", "-m", mM, "-i", i}; } 
				else { argsV = {"./server", "-m", mM}; }
		} else {
			if (i_) { argsV = {"./server", "-i", i}; } 
				else { argsV = {"./server"}; }
		}
		
		int sz(argsV.size());
		byte_t* args[sz+1];
        for (int i(0); i < sz; i++) { 
            args[i] = const_cast<byte_t*>(argsV.at(i).c_str());
        }
		args[sz] = NULL; 

		execvp(args[0],args);
		throw Oopsie_Daisies("./server");

	} else { /* [PARENT] CLIENT */
		byte_t *nCBuf(new byte_t[m]);
		RequestChannel* chan(chanT(m,i));
		RequestChannel* ctrl(chan);
		std::vector<RequestChannel*> channels;
		for (int j = 0; j < c; j++) { channels.push_back(newChan(ctrl, nCBuf, m, i)); }

		struct timeval start, end;
		gettimeofday(&start, 0);
		
		if (PTE) { /* [Single Data Point Request] */
			double reply(0);
			byte_t* buf(new byte_t[m]);
			datamsg* dm(new datamsg(p,t,e));
			std::memcpy(buf, dm, sizeof(datamsg));

			dmRequest(ctrl, buf, reply);
			Print1DP(p, t, e, reply);

			delete[] buf;
			delete dm;

		} else if (ONLY_P) { /* [Data Points -> CSV Request] */
			if (c_) { 
				int n(0);
				for (auto i : channels) { 
					CSVTrans(i, m, p, ++n); 
				}

			} else { 
				CSVTrans(ctrl, m, p);
			}

		} else if (f_) { /* [File Tranfer Request] */
			if (c_) { 
				fileTrans(channels, m, filename, ctrl);

			} else { 
				std::vector<RequestChannel*> tmp = {ctrl};
				fileTrans(tmp, m, filename, ctrl);

			}
		}

		gettimeofday(&end, 0);

		int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
		int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
		std::cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << std::endl;

		for (auto i : channels) { delChan(i); }
		delete[] nCBuf;
		channels.clear();
		delChan(ctrl);
		wait(&stat);
	}
}
