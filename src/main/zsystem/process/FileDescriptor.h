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

#ifndef ZSYSTEM_PROCESS_FILEDESCRIPTOR_H_
#define ZSYSTEM_PROCESS_FILEDESCRIPTOR_H_

#include <utility>
#include <string>

//#include <unistd.h>

namespace zsystem {
namespace process {

class FileDescriptor {
public:
	using Handle = int;

    static const Handle noHandle;
    static const Handle stdInHandle;
    static const Handle stdOutHandle;
    static const Handle stdErrHandle;

    static const std::size_t npos;

	static std::pair<FileDescriptor, FileDescriptor> openUnidirectional();
	static std::pair<FileDescriptor, FileDescriptor> openBidirectional();
	static FileDescriptor openFile(const std::string& filename, bool isRead, bool isWrite, bool doOverwrite);

	FileDescriptor() = default;
	FileDescriptor(const FileDescriptor&) = delete;
	FileDescriptor(FileDescriptor&& other);
	~FileDescriptor();

	FileDescriptor& operator=(const FileDescriptor&) = delete;
	FileDescriptor& operator=(FileDescriptor&& other);

	explicit operator bool() const noexcept;

	Handle getHandle() const noexcept;
	Handle release() noexcept;

	std::size_t read(void* data, std::size_t size);
	std::size_t write(const void* data, std::size_t size);
	std::size_t getFileSize() const;

	void close();

	/* return true on success, false on error */
	bool setBlocking(bool b);

private:
	FileDescriptor(Handle fd);
	int fd = noHandle;
};

} /* namespace process */
} /* namespace zsystem */

#endif /* ZSYSTEM_PROCESS_FILEDESCRIPTOR_H_ */
