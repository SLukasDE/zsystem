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

#ifndef ZSYSTEM_SIGNALHANDLER_H_
#define ZSYSTEM_SIGNALHANDLER_H_

#include <functional>

namespace zsystem {

/* there are two kinds of installing and removing a signal handler.
 *
 * First version is to derive a class from SignalHandler, implement method invoke and create an instance.
 * The signal handler will be removed by destroying this object again.
 *
 * Second version is to call static methods install and remove.
 *
 * Both versions must be called from same thread or you have to guarantee that no two threads are doing this at the same time.
 * in general you should install signal handlers at the beginning of your process and destroy then at the end (if needed).
 * You should also not manipulate signal handler installation if an signal can be called. There is a data structure in background used
 * by signal handlers that is manipulated by installing or removing a signal handler.
 */

class SignalHandler {
public:
	enum class Type {
	    unknown,
		hangUp,
	    interrupt,
	    quit,
		ill,
		trap,
		abort,
		busError,
		floatingPointException,
		segmentationViolation,
		user1,
		user2,
		alarm,
		child,
		stackFault,
	    terminate,
	    pipe
	};

    SignalHandler(Type signalType);
    virtual ~SignalHandler();

    static void install(Type signalType, std::function<void()> handler);
    static void remove(Type signalType, std::function<void()> handler);

    static void handle(Type signalType);

protected:
    virtual void invoke() = 0;

private:
    Type type;
};

} /* namespace zsystem */

#endif /* ZSYSTEM_SIGNALHANDLER_H_ */
