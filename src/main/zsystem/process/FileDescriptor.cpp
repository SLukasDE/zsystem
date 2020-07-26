/*
MIT License
Copyright (c) 2019, 2020 Sven Lukas

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

#include <zsystem/process/FileDescriptor.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <cstring>
#include <stdexcept>
#include <iostream>

namespace zsystem {
namespace process {

const FileDescriptor::Handle FileDescriptor::noHandle = -1;
const FileDescriptor::Handle FileDescriptor::stdInHandle = STDIN_FILENO; // 0
const FileDescriptor::Handle FileDescriptor::stdOutHandle = STDOUT_FILENO; // 1
const FileDescriptor::Handle FileDescriptor::stdErrHandle = STDERR_FILENO; // 2

const std::size_t FileDescriptor::npos = static_cast<std::size_t>(-1);

std::pair<FileDescriptor, FileDescriptor> FileDescriptor::openUnidirectional() {
	int pipeFd[2];
	//if(pipe2(pipeFd, O_NONBLOCK) == -1) {
	if(pipe2(pipeFd, 0) == -1) {
		throw std::runtime_error(std::string("FileDescriptor::createPipe() failed: ") + std::strerror(errno));
	}

	return std::make_pair(FileDescriptor(pipeFd[0]), FileDescriptor(pipeFd[1]));
}

std::pair<FileDescriptor, FileDescriptor> FileDescriptor::openBidirectional() {
	int socketFd[2];
	if(socketpair(AF_UNIX, SOCK_STREAM, 0, socketFd) == -1) {
		throw std::runtime_error(std::string("FileDescriptor::createPipe() failed: ") + std::strerror(errno));
	}

	return std::make_pair(FileDescriptor(socketFd[0]), FileDescriptor(socketFd[1]));
}

FileDescriptor FileDescriptor::openFile(const std::string& filename, bool isRead, bool isWrite, bool doOverwrite) {
	if(isRead || isWrite) {
		int flags = O_NOCTTY;

		if(!isWrite) {
			flags |= O_RDONLY;
		}
		else if(!isRead) {
			flags |= O_WRONLY;
			flags |= O_CREAT;
		}
		else {
			flags |= O_RDWR;
			flags |= O_CREAT;
		}

		if(isWrite) {
			if(doOverwrite) {
				flags |= O_TRUNC;
			}
			else {
				flags |= O_APPEND;
			}
		}

		while(true) {
			int fd = open(filename.c_str(), flags, 0644);
			if(fd != -1) {
				return FileDescriptor(fd);
			}
			else if(errno == EINTR) {
				continue;
			}
			throw std::runtime_error("FileDescriptor::openFile(\"" + filename + "\", ...) failed: " + std::strerror(errno));
		}
	}

	return FileDescriptor();
}

FileDescriptor::FileDescriptor(FileDescriptor&& other)
: fd(other.fd)
{
	other.fd = noHandle;
}

FileDescriptor::FileDescriptor(int aFd)
: fd(aFd)
{ }

FileDescriptor::~FileDescriptor() {
	close();
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) {
	if(this != &other) {
		/* first delete own entries */
		close();

		/* now move entries */
		fd = other.fd;

		other.fd = noHandle;
	}

	return *this;
}

FileDescriptor::operator bool() const noexcept {
	return fd != noHandle;
}

FileDescriptor::Handle FileDescriptor::getHandle() const noexcept {
	return fd;
}

FileDescriptor::Handle FileDescriptor::release() noexcept {
	Handle rv = fd;
	fd = noHandle;
	return rv;
}

std::size_t FileDescriptor::read(void* data, std::size_t size) {
	if(fd == noHandle) {
		return npos;
	}

	ssize_t count;
	while(true) {
		count = ::read(fd, data, size);
		if(count != -1 || errno != EINTR) {
			break;
		}
	}
	return count == -1 ? npos : count;
}

std::size_t FileDescriptor::write(const void* data, std::size_t size) {
	if(fd == noHandle) {
		return npos;
	}

	ssize_t count;
	while(true) {
		count = ::write(fd, data, size);
		if(count != -1 || errno != EINTR) {
			break;
		}
	}
	return count == -1 ? npos : count;
}

std::size_t FileDescriptor::getFileSize() const {
	if(getHandle() != process::FileDescriptor::noHandle) {
        struct stat statBuffer;
        if(fstat(getHandle(), &statBuffer) == 0) {
        	return static_cast<std::size_t>(statBuffer.st_size);
        }
    }

	return process::FileDescriptor::npos;
}

void FileDescriptor::close() {
	if(fd != noHandle) {
		while(::close(fd) == EINTR) { }
		fd = noHandle;
	}
}

bool FileDescriptor::setBlocking(bool b) {
	if(fd == noHandle) {
		return false;
	}

	int flags = 0;
	while(true) {
		flags = fcntl(fd, F_GETFL, 0);
		if(flags != -1) {
			break;
		}
		else if(errno != EINTR) {
			std::cerr << "fcntl(fd, F_GETFL, 0) failed for fd=" << fd << "\n";
			return false;
		}
	}

	if(b) {
		flags = (flags & ~O_NONBLOCK);
	}
	else {
		flags |= O_NONBLOCK;
	}

	if(flags != -1) {
		while(fcntl(fd, F_SETFL, flags) == -1) {
			if(errno != EINTR) {
				std::cerr << "fcntl(fd, F_SETFL, flags) failed for fd=" << fd << " and flags=" << flags << "\n";
				return false;
			}
		}
	}
	return true;
}

} /* namespace process */
} /* namespace zsystem */
