/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Archer Simmons
	UIN: 726004511
	Date: 2/4/22
*/
#include "common.h"
#include <sys/wait.h>
#include "FIFORequestChannel.h"

using namespace std;

#define DPRQ 1000 // For requesting file message 1000 bytes in length
#define ONLY_P (p_ && !(t_||e_||f_||c_)) // bool for only p passes as arg
#define PTE (p_ && t_ && e_)
#define MM_I64_CAST (__int64_t)MAX_MESSAGE
#define fm_ filemsg*


int main (int argc, char *argv[]) {
	bool p_(0), t_(0), e_(0), f_(0), m_(0), c_(0); 
	int opt, p(1), e(1), maxMsg(MAX_MESSAGE), stat;
	std::string filename = "";
	double t(1);
	byte_t* optarg_m;

	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				p_ = 1;
				break;
			case 't':
				t = atof (optarg);
				t_ = 1;
				break;
			case 'e':
				e = atoi (optarg);
				e_ = 1;
				break;
			case 'f':
				filename = optarg;
				f_ = 1;
				break;
			case 'm':
				maxMsg = atoi(optarg);
				optarg_m = optarg;
				m_ = 1;
				break;
			case 'c': 
				c_ = 1; 
				break;
			default: 
				throw Fried_Cheese("getopt() switch default");
				break;
		}
	}

	pid_t ID = fork();

	if (ID == 0) { // CHILD (SERVER)
		if (m_) {
			byte_t* arg[] = {(byte_t*)"./server", (byte_t*)"-m", optarg_m, NULL};
			execvp(arg[0],arg);
			EXITONERROR("./server -m");
		} else {
			byte_t* arg[] = {(byte_t*)"./server", NULL};
			execvp(arg[0],arg);
			EXITONERROR("./server");
		}

	} else { // PARENT

		byte_t* buf(new byte_t[MAX_MESSAGE]);
		FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
		
		if (PTE) { //Single data point $./client -p <pt no> -t <seconds> -e <ecg no>
			double reply(0.0);
			datamsg dm(p,t,e);
			memcpy(buf, &dm, sizeof(datamsg));

			if(c_) {
				byte_t* nCBuf(new byte_t[maxMsg]);
				// Open new channel
				MESSAGE_TYPE m(NEWCHANNEL_MSG);
				chan.cwrite(&m, sizeof(MESSAGE_TYPE));
				chan.cread(nCBuf, maxMsg);
				FIFORequestChannel nChan(nCBuf, FIFORequestChannel::CLIENT_SIDE);

				nChan.cwrite(buf, sizeof(datamsg)); 
				nChan.cread(&reply, sizeof(double));

				MESSAGE_TYPE mqx(QUIT_MSG); 
				nChan.cwrite(&mqx, sizeof(MESSAGE_TYPE)); 
				
				delete[] nCBuf;

			} else {
				chan.cwrite(buf, sizeof(datamsg)); 
				chan.cread(&reply, sizeof(double)); 
			}

			cout << "For person " << p << ", at time " << t 
				<< ", the value of ecg " << e << " is " << reply << endl;

		} else if (ONLY_P) { // 1000 data point $./client -p <pt no>

			byte_t* buf2(new byte_t[maxMsg]);
			double rep2(0.0), time(0.0), reply(0.0);;
			ofstream DP1K("received/x1.csv");

			if (c_) {
				byte_t* nCBuf(new byte_t[maxMsg]);
				MESSAGE_TYPE m(NEWCHANNEL_MSG);
				chan.cwrite(&m, sizeof(MESSAGE_TYPE));
				chan.cread(nCBuf, maxMsg);
				FIFORequestChannel nChan(nCBuf, FIFORequestChannel::CLIENT_SIDE);

				while(time < 4) {
					datamsg dm(p,time,1), dm2(p,time,2);
					memcpy(buf, &dm, sizeof(datamsg));
					nChan.cwrite(buf, sizeof(datamsg));
					nChan.cread(&reply, sizeof(double));
					memcpy(buf2, &dm2, sizeof(datamsg));
					nChan.cwrite(buf2, sizeof(datamsg));
					nChan.cread(&rep2, sizeof(double));
					DP1K << time << "," << reply << "," << rep2 << std::endl;
					time += 0.004;
				}

				MESSAGE_TYPE mqx(QUIT_MSG); 
				nChan.cwrite(&mqx, sizeof(MESSAGE_TYPE)); 
				
				delete[] nCBuf;

			} else {

				while(time < 4) {
					datamsg dm(p,time,1), dm2(p,time,2);
					memcpy(buf, &dm, sizeof(datamsg));
					chan.cwrite(buf, sizeof(datamsg));
					chan.cread(&reply, sizeof(double));
					memcpy(buf2, &dm2, sizeof(datamsg));
					chan.cwrite(buf2, sizeof(datamsg));
					chan.cread(&rep2, sizeof(double));
					DP1K << time << "," << reply << "," << rep2 << std::endl;
					time += 0.004;
				}

			}

			DP1K.close();
			delete[] buf2;

		} else if (f_) { // $./client -f <file name>

			filemsg* fm(new filemsg(0,0));
			__int64_t fSize;
			byte_t* readBuf(new byte_t[maxMsg]);

			std::string fo("received/" + filename);
			int len = sizeof(filemsg) + (filename.size() + 1);  
			byte_t* buf2(new byte_t[len]);
			memcpy(buf2, fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), filename.c_str());
			FILE* file(fopen(fo.c_str(), "wb"));

			if (c_) { // NEW CHANNEL REQUESTED FOR FILE TRANSFER

				// Open new channel
				MESSAGE_TYPE m(NEWCHANNEL_MSG);
				chan.cwrite(&m, sizeof(MESSAGE_TYPE));
				chan.cread(buf, maxMsg);
				FIFORequestChannel nChan(buf, FIFORequestChannel::CLIENT_SIDE);

				// Get file size
				nChan.cwrite(buf2, len);  
				nChan.cread(&fSize, sizeof(__int64_t));

				// Transfer file
				__int64_t bytes(1), offset_(0), remainder(fSize); /////////////TRY CHANGING TO INT
				filemsg* fileTrans(new filemsg(offset_, bytes));
				while(bytes > 0) { 

					bytes = std::min((__int64_t)maxMsg, remainder); ////////////////////
					fileTrans->offset = offset_;
					fileTrans->length = bytes;
					memcpy(buf2, fileTrans, sizeof(filemsg));
					nChan.cwrite(buf2, len);
					nChan.cread(readBuf, bytes);
					fwrite(readBuf, 1, bytes, file);
					remainder -= bytes;
					offset_ += bytes; 
				}

				delete fileTrans;
				MESSAGE_TYPE mqx(QUIT_MSG); 
				nChan.cwrite(&mqx, sizeof(MESSAGE_TYPE)); 

			} else { // FILE TRANSFER USING CHAN

				chan.cwrite(buf2, len);  
				chan.cread(&fSize, sizeof(__int64_t));

				// Transfer file
				__int64_t bytes(1), offset_(0), remainder(fSize); 
				filemsg* fileTrans(new filemsg(offset_, bytes));
				while(bytes > 0) { 
				
					bytes = std::min((__int64_t)maxMsg, remainder);
					fileTrans->offset = offset_;
					fileTrans->length = bytes;
					memcpy(buf2, fileTrans, sizeof(filemsg));
					chan.cwrite(buf2, len);
					chan.cread(readBuf, bytes);
					fwrite(readBuf, 1, bytes, file);
					remainder -= bytes;
					offset_ += bytes; 
					
				}
				delete fileTrans;
			}
			fclose(file);
			delete[] buf2;
			delete[] readBuf;
			delete fm;
		}

		// Housekeeping
		delete[] buf;
		MESSAGE_TYPE mqx(QUIT_MSG); 
		chan.cwrite(&mqx, sizeof(MESSAGE_TYPE));
		wait(&stat);
	}
}
