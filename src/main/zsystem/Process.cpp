/*
MIT License
Copyright (c) 2019 Sven Lukas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <zsystem/Process.h>
#include <iostream>
#include <string.h>
#include <fcntl.h>  /* fuer O_RDWR|O_CREAT */
#include <errno.h>  /* fuer errno != EINTR */
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/mman.h>

namespace zsystem {

namespace {
char** createArgs(const std::string& executable, const std::list<std::string>& arguments) {
    char** argv = new char*[arguments.size()+2];

    argv[0] = new char[executable.size()+1];
    argv[0][0] = 0;
    strcat(argv[0], executable.c_str());

    int i = 1;
    for(std::list<std::string>::const_iterator iter = arguments.begin(); iter != arguments.end(); ++iter) {
        argv[i] = new char[iter->size()+1];
        argv[i][0] = 0;
        strcat(argv[i], iter->c_str());
        ++i;
    }

    argv[i] = nullptr;
    return argv;
}

} /* anonymous namespace */



Process::File::File(std::string aFilename)
: Output(),
  filename(std::move(aFilename))
{
}

Process::File::~File() {
	close();
}

bool Process::File::open() {
	if(fdFile != -1) {
		return false;
	}
    fdFile = ::open(filename.c_str(), O_WRONLY|O_APPEND|O_CREAT, 0666);
    if(fdFile == -1) {
    	std::cerr << "Cannot open output file \"" << filename << "\"\n";
    	return false;
    }
    return true;
}

void Process::File::join(int fd) {
    ::close(fd);
	if(fdFile != -1) {
	    dup2(fdFile, fd);
	}
}

void Process::File::joined(bool) {
}

void Process::File::close() {
	if(fdFile != -1) {
	    ::close(fdFile);
	    fdFile = -1;
	}
}

std::size_t Process::File::read(void* buffer, std::size_t s) {
	return 0;
}

bool Process::File::setBlocking(bool blocking) {
	return true;
}




Process::Pipe::Pipe()
: Output()
{
	pipefd[0] = -1;
	pipefd[1] = -1;
}

Process::Pipe::~Pipe() {
	close();
}

bool Process::Pipe::open() {
	if(pipefd[0] != -1 || pipefd[1] != -1) {
		return false;
	}

    if(pipe2(pipefd, O_NONBLOCK) == -1) {
    	pipefd[0] = -1;
    	pipefd[1] = -1;
    	std::cerr << "Cannot open pipe\n";
    	return false;
    }

	readFlags = fcntl(pipefd[0], F_GETFL, 0);
	return true;
}

void Process::Pipe::join(int fd) {
//    ::close(fd);
	if(pipefd[1] != -1) {
		while ((dup2(pipefd[1], fd) == -1) && (errno == EINTR)) {
		}
	}
}

void Process::Pipe::joined(bool isParent) {
	if(pipefd[0] != -1 && isParent == false) {
		::close(pipefd[0]);
		pipefd[0] = -1;
	}

	if(pipefd[1] != -1) {
		::close(pipefd[1]);
		pipefd[1] = -1;
	}
}

void Process::Pipe::close() {
	joined(false);
}

std::size_t Process::Pipe::read(void* buffer, std::size_t s) {
	if(pipefd[0] == -1) {
		return 0;
	}

	ssize_t rv = ::read(pipefd[0], buffer, s);
	if(rv == -1) {
		return 0;
	}
	return rv;
}

bool Process::Pipe::setBlocking(bool blocking) {
	if(pipefd[0] == -1) {
		return false;
	}
#ifdef _WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#else
	if(readFlags == -1) {
		return false;
	}
	readFlags = blocking ? (readFlags & ~O_NONBLOCK) : (readFlags | O_NONBLOCK);
   return fcntl(pipefd[0], F_SETFL, readFlags) == 0;
#endif
}




bool Process::Default::open() {
	return true;
}

void Process::Default::join(int) {
}

void Process::Default::joined(bool) {
}

void Process::Default::close() {
}

std::size_t Process::Default::read(void*, std::size_t) {
	return 0;
}

bool Process::Default::setBlocking(bool blocking) {
	return true;
}










long double Process::clktck = static_cast<long double>(sysconf(_SC_CLK_TCK)) / 1000;
int Process::fdNull = open("/dev/null", O_RDWR);

Process::~Process() {
    wait();

    if(argv != nullptr) {
        for(int i = 0; argv[i] != nullptr; ++i) {
            delete[] argv[i];
        }
    }
}

void Process::enableTimeMeasurement(bool enabled) {
	if(running == false) {
		doTimeMeasurement = enabled;
	}
}

void Process::setWorkingDirectory(const std::string& aWorkingDirectory) {
	workingDirectory = aWorkingDirectory;
}

void Process::setStdOut(Output* output) {
	outPtr = output;
}

void Process::setStdErr(Output* output) {
	errPtr = output;
}

bool Process::execute(const std::string& executable, const std::list<std::string>& arguments) {
    if(running) {
        return false;
    }

    failed = true;

    argv = createArgs(executable, arguments);
    const char* chdirStr = nullptr;
    if(!workingDirectory.empty()) {
        chdirStr = workingDirectory.c_str();
    }

    if(outPtr) {
    	if(outPtr->open() == false) {
    		return false;
    	}
    }
    if(errPtr && errPtr != outPtr) {
    	if(errPtr->open() == false) {
    		return false;
    	}
    }

    /* Variablen, die erst im Time-Prozess benoetigt werden.
     * Wir legen sie bereits hier an, damit im geforkten Prozess nicht
     * womoeglich der Stack vergroessert werden muss
     */
    pid_t timerPid = 0;
    timeval tv;
    suseconds_t realTimeStartUsec;
    struct tms startTms;
    struct tms endTms;
    double realMs;
    double cuser;
    double csystem;
    int returnVal;
    pid_t rcWaitPid = 0;

    if(doTimeMeasurement) {
    	timeDataMemory.reset(new SharedMemory<TimeData>);
        if(timeDataMemory->getData() == nullptr) {
        	timeDataMemory.reset();
        	std::cerr << "Mmap failed.\n";
            return false;
        }
    }

    pid = fork();

    /* fork failed */
    if(pid < 0) {
    	timeDataMemory.reset();
        std::cerr << "Fehler bei der Ausfuehrung von fork() aufgetreten\n";
        return false;
    }

    if (pid == 0) { /* child */

        /* ********************************** */
        /* STDIN entsprechend Child-IN setzen */
        /* ********************************** */

        close(STDIN_FILENO);
        if(fdNull != -1) {
            dup2(fdNull, STDIN_FILENO);
        }

        /* *********************************************** */
        /* STDOUT und STDERR entsprechend Child-OUT setzen */
        /* *********************************************** */

        if(outPtr) {
        	outPtr->join(STDOUT_FILENO);
        }
        else {
            close(STDOUT_FILENO);
        }

        if(errPtr) {
        	errPtr->join(STDERR_FILENO);
        }
        else {
            close(STDERR_FILENO);
        }

        if(outPtr) {
        	outPtr->joined(false);
        }
        if(errPtr) {
        	errPtr->joined(false);
        }
//            setsid();
//            ioctl(STDIN_FILENO, TIOCSCTTY, 1);

        if(chdirStr && chdir(chdirStr) == -1) {
            write(STDERR_FILENO, "Unable to change to directory \"", strlen("Unable to change to directory \""));
            write(STDERR_FILENO, chdirStr, strlen(chdirStr));
            write(STDERR_FILENO, "\"", strlen("\""));
            _exit(-2);
//            exit(EXIT_FAILURE);
        }

    	timerPid = 0;
        if(doTimeMeasurement) {
            gettimeofday(&tv, nullptr);
            realTimeStartUsec = tv.tv_sec * 1000000 + tv.tv_usec;
            times(&startTms);

            timerPid = fork();
            if (timerPid < 0) { /* error */
                _exit(-2);
//                exit(EXIT_FAILURE);
            }
        }

        if (timerPid == 0) { /* child */

            /* Terminate child, wenn parent killed, use once only !!!! */
            // int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
            prctl(PR_SET_PDEATHSIG, SIGTERM);

            execv(argv[0], argv);
    		perror("execv");
    		_exit(1);
        }

        timeDataMemory->getData()->pid = timerPid;

        /* timer Parent-Process */
        while(true) {
            // in case we are reading from the child, we have to return from waitpid
            // otherwise it might lead to a deadlock in case the child does not terminate
            // because its output is not being consumed.
        	rcWaitPid = waitpid(timerPid, &returnVal, 0);

            gettimeofday(&tv, nullptr);
            times(&endTms);

            realMs = (tv.tv_sec * 1000000 + tv.tv_usec) - realTimeStartUsec;
            realMs /= 1000;
        	timeDataMemory->getData()->realMs = realMs;

            cuser = endTms.tms_cutime - startTms.tms_cutime;
            cuser /= clktck;
            timeDataMemory->getData()->userMs = cuser;

            csystem = endTms.tms_cstime - startTms.tms_cstime;
            csystem /= clktck;
            timeDataMemory->getData()->sysMs = csystem;

            if(rcWaitPid == -1) {
                /* on error */
                std::cerr << "waitpid: error occurs, signal interruped ..." << std::endl;
                returnVal = EXIT_FAILURE;
                break;
            }

            if(WIFEXITED(returnVal)) {
                returnVal = WEXITSTATUS(returnVal);
                break;
            }
            if(WIFSIGNALED(returnVal)) {
                // follow the same convention as bash of returning signal values in return codes by adding 128 to them
                returnVal = 128 + WTERMSIG(returnVal);
                break;
            }
        }

        _exit(returnVal);
//        exit(returnVal);
    }

    /* parent continues... */
    running = true;
    failed = false;

    if(outPtr) {
    	outPtr->joined(true);
    }
    if(errPtr) {
    	errPtr->joined(true);
    }

    return true;
}

int Process::wait() {
    while(running) {
        // in case we are reading from the child, we have to return from waitpid
        // otherwise it might lead to a deadlock in case the child does not terminate
        // because its output is not being consumed.
    	updateStatus(true);
    }

    return exitStatus;
}

bool Process::isRunning() {
	updateStatus(false);
    return running;
}

bool Process::isFailed() {
	updateStatus(false);
    return failed;
}

pid_t Process::getPid() const {
	if(timeDataMemory) {
	    return timeData.pid;
	}
    return pid;
}

unsigned int Process::getTimeRealMS() const {
    return timeData.realMs;
}

unsigned int Process::getTimeUserMS() const {
    return timeData.userMs;
}

unsigned int Process::getTimeSysMS() const {
    return timeData.sysMs;
}

/* ******************************************** */
/* *** PRIVATE                              *** */
/* ******************************************** */

void Process::updateStatus(bool doWait) {
    if(!running) {
        return;
    }

    int tmpStatus;
    int rc = waitpid(pid, &tmpStatus, doWait ? 0 : WNOHANG);

    // should never happen if "doWait == false"
    if(rc == -1) {
    	// ERROR
        return;
    }

    if(timeDataMemory) {
        timeData = *timeDataMemory->getData();
    }

    // should never happen if "doWait == true"
    // no status availabe
    if(rc == 0) {
        return;
    }

    /*
    if(WIFSTOPPED(tmpStatus)) {
    }
    if(WIFCONTINUED(tmpStatus)) {
    }
    */

    if(WIFSIGNALED(tmpStatus)) {
        // follow the same convention as bash of returning signal values in return codes by adding 128 to them
        exitStatus = 128 + WTERMSIG(tmpStatus);
        failed = true;

        running = false;
    	timeDataMemory.reset();
        if(outPtr) {
        	outPtr->close();
        }
        if(errPtr) {
        	errPtr->close();
        }
    }
    if(WIFEXITED(tmpStatus)) {
        exitStatus = WEXITSTATUS(tmpStatus);

        running = false;
    	timeDataMemory.reset();
        if(outPtr) {
        	outPtr->close();
        }
        if(errPtr) {
        	errPtr->close();
        }
    }
}

} /* namespace zsystem */
