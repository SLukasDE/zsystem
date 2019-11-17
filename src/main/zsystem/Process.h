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

#ifndef ZSYSTEM_PROCESS_H_
#define ZSYSTEM_PROCESS_H_

#include <zsystem/SharedMemory.h>
#include <unistd.h>
#include <string>
#include <list>
#include <memory>

namespace zsystem {

class Process {
public:
    class Output {
    friend class Process;
    public:
    	Output() = default;
    	virtual ~Output() = default;

    	virtual std::size_t read(void* buffer, std::size_t s) = 0;
    	virtual bool setBlocking(bool blocking) = 0;

    private:
    	virtual bool open() = 0;
    	virtual void join(int fd) = 0;
    	virtual void joined(bool isParent) = 0;
    	virtual void close() = 0;
    };

    class File : public Output {
    public:
    	File(std::string filename);
    	~File();

    	bool open() override;
    	void join(int fd) override;
    	void joined(bool) override;
    	void close() override;

    	std::size_t read(void* buffer, std::size_t s) override;
    	bool setBlocking(bool blocking) override;

    private:
    	std::string filename;
    	int fdFile = -1;
    };

    class Pipe: public Output {
    public:
    	Pipe();
    	~Pipe();


    	bool open() override;
    	void join(int fd) override;
    	void joined(bool isParent) override;
    	void close() override;

    	std::size_t read(void* buffer, std::size_t s) override;
    	bool setBlocking(bool blocking) override;

    private:
    	int pipefd[2];
    	int readFlags = 0;
    };

    class Default : public Output {
    public:
    	Default() = default;
    	~Default() = default;

    	bool open() override;
    	void join(int) override;
    	void joined(bool) override;
    	void close() override;

    	std::size_t read(void*, std::size_t);
    	bool setBlocking(bool blocking);
    };

    Process() = default;
    ~Process();

    void enableTimeMeasurement(bool enabled);
    void setWorkingDirectory(const std::string& workingDirectory);

    void setStdOut(Output* output);
    void setStdErr(Output* output);

    /* return true on success */
    bool execute(const std::string& executable, const std::list<std::string>& arguments);

    int wait();
    bool isRunning();
    bool isFailed();

    unsigned int getTimeRealMS() const;
    unsigned int getTimeUserMS() const;
    unsigned int getTimeSysMS() const;

    pid_t getPid() const;

private:
    struct TimeData {
        unsigned int realMs = 0;
        unsigned int userMs = 0;
        unsigned int sysMs = 0;
        pid_t pid = 0;
    };

    void updateStatus(bool doWait);


    std::unique_ptr<Output> outErr;
    std::unique_ptr<Output> err;

    Output* outPtr = nullptr;
    Output* errPtr = nullptr;

    bool doTimeMeasurement = false;
    std::string workingDirectory;

    static int fdNull;
    pid_t pid = 0;
    bool running = false;
    bool failed = false;
    int exitStatus = -1;
    char** argv = nullptr;

    static long double clktck;
    TimeData timeData;
    std::unique_ptr<SharedMemory<TimeData>> timeDataMemory;
};

} /* namespace zsystem */

#endif /* ZSYSTEM_PROCESS_H_ */
