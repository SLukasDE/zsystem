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

#ifndef ZSYSTEM_SHAREDMEMORY_H_
#define ZSYSTEM_SHAREDMEMORY_H_

#include <string>

namespace zsystem {

template<class T>
class SharedMemory;

template<>
class SharedMemory<void> {
public:
	SharedMemory(std::size_t sharedMemSize);
	~SharedMemory();

	void* getData();

private:
	void* sharedMemPtr = nullptr;
	std::size_t sharedMemSize = 0;
};


template<class T>
class SharedMemory {
public:
	SharedMemory()
    : systemSharedMemory(sizeof(T))
    {
		T* t = reinterpret_cast<T*> (systemSharedMemory.getData());
		new (t) T; // placement new
    }

	~SharedMemory() = default;

	T* getData() {
		return static_cast<T*>(systemSharedMemory.getData());
	}

private:
	SharedMemory<void> systemSharedMemory;
};


} /* namespace zsystem */

#endif /* ZSYSTEM_SHAREDMEMORY_H_ */
