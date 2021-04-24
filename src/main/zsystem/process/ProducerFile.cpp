/*
MIT License
Copyright (c) 2019-2021 Sven Lukas

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

#include <zsystem/process/ProducerFile.h>

namespace zsystem {
namespace process {

ProducerFile::ProducerFile(FileDescriptor aFileDescriptor)
: fileDescriptor(std::move(aFileDescriptor))
{ }

std::size_t ProducerFile::produce(FileDescriptor& fileDescriptor) {
	if(currentPos >= currentSize) {
		currentPos = 0;
		currentSize = getFileDescriptor().read(buffer, sizeof(buffer));
	}

	if(currentSize == 0 || currentSize == process::FileDescriptor::npos) {
		return process::FileDescriptor::npos;
	}

	std::size_t count = fileDescriptor.write(&buffer[currentPos], currentSize - currentPos);
	if(count != process::FileDescriptor::npos) {
		currentPos += count;
	}

	return count;
}

std::size_t ProducerFile::getFileSize() const {
	return fileDescriptor.getFileSize();
}

FileDescriptor& ProducerFile::getFileDescriptor() & {
	return fileDescriptor;
}

FileDescriptor&& ProducerFile::getFileDescriptor() && {
	return std::move(fileDescriptor);
}

} /* namespace process */
} /* namespace zsystem */
