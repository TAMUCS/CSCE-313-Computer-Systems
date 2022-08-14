#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ctime>
#include <vector>
#include <string>
#include <fcntl.h> 
#include <errno.h>
#include <algorithm>
#include <utmp.h>
#include <stdio.h> 
#include <pwd.h>
#include <string.h>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

struct Fried_Cheese : std::logic_error {
    Fried_Cheese(std::string e, std::string d=" Dun Goofed My Guy\nInitializing Hoon Crank Proccess...") 
        : std::logic_error(e+d) {}
};

int main () {

    // Restore copies of std{i/o}:
    int save_i = dup(STDIN_FILENO);
    int save_o = dup(STDOUT_FILENO);

    // Vector of months for int->month conversion:
    const std::vector<std::string> M{"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    
    // Absolute directory:
    char buf[FILENAME_MAX];
    char* succ(getcwd(buf, FILENAME_MAX));
    if(!succ) { throw Fried_Cheese("getcwd()"); }
    
    std::string currDir(succ);
    std::string prevDir(currDir);

    for (;;) {

        // Time:
        time_t now(time(0));
        tm *ltm(localtime(&now));

        // Username:
        struct passwd *pass; 
        pass = getpwuid(getuid()); 
        char* name(pass->pw_name);

        cout << YELLOW << M.at(ltm->tm_mon) << " " << ltm->tm_mday << " " << ltm->tm_hour << ":" 
                << ltm->tm_min << ":" << ltm->tm_sec << " " << std::string(name) << ":" << std::string(currDir)
                << "$" << NC << " ";
        
        // get user inputted command
        std::string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }

        // CD:
        if (tknr.commands.at(0)->args.at(0) == "cd") { // IF NOT WORK, USE .AT(2)
            if (tknr.commands.at(0)->args.at(1) == "-") {
                currDir = prevDir;
                if (chdir(prevDir.c_str()) != 0) {
                    throw Fried_Cheese("Chdir()");
                }
            } else {
                prevDir = currDir;
                if (chdir((char*)tknr.commands.at(0)->args.at(1).c_str()) != 0) {
                    throw Fried_Cheese("Chdir()");
                }
                memset(buf, 0, sizeof(buf));
                currDir = std::string(getcwd(buf, FILENAME_MAX));
            }
            continue;
        }

        // For loop for handeling piped commands:
        int pFds[2], nFds[2];
        for (auto cmd = tknr.commands.begin(); cmd != tknr.commands.end(); ++cmd) {
            auto n(((*cmd)->args).size());
            int inFds, outFds;
            bool first(0), last(0);
            first = (cmd == tknr.commands.begin()) ? 1:0;  
            last = (std::next(cmd) == tknr.commands.end()) ? 1:0;

            if (!last) {
                if(pipe(nFds) < 0) {
                    perror("pipe");
                    exit(2);
                }
            }
            // fork to create child
            pid_t pid(fork());
            if (pid < 0) {  // error check
                perror("fork");
                exit(2);
            }

            if (pid == 0) {  // if child, exec to run command
                // iterates over args of current command to make char* array, then pass to execvp()
                char** args = new char*[n];
                for (size_t i = 0; i < n; i++) {
                    args[i] = const_cast<char*>(((*cmd)->args).at(i).c_str());
                }

                // if current command is redirected, then open file & dup2 std{in/out}
                if ((*cmd)->hasInput()) { // '<'
                    inFds = (open((*cmd)->in_file.c_str(), O_RDONLY));
                    dup2(inFds, 0);
                    close (inFds);
                } 
                if ((*cmd)->hasOutput()) { // '>'
                    outFds = open((*cmd)->out_file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
                    dup2(outFds, 1);
                    close(outFds);
                }
                if(!first) {
                    dup2(pFds[0],0);
                    close(pFds[1]);
                }
                if (!last) {
                    dup2(nFds[1],1);
                    close(nFds[0]);
                }
                if (execvp(args[0], args) < 0) {  // error check
                    perror("execvp");
                    exit(2);
                }
                delete[] args;
            } else {  // if parent, wait for child to finish
                if(!first) {
                    close(pFds[0]);
                    close(pFds[1]);
                }
                if (!last) {
                    pFds[0] = nFds[0];
                    pFds[1] = nFds[1];
                }
                int status(0);
                waitpid(pid, &status, 0);
                if (status > 1) {  // exit if child didn't exec properly
                    exit(status);
                }
            }
        }
        // Restores std{i/o}:
        dup2(save_i, STDIN_FILENO);
        dup2(save_o, STDOUT_FILENO);
    }
}