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

#include <zsystem/Signal.h>

#include <signal.h>

namespace zsystem {


void Signal::sendSignal(Process::Handle handle, Type signal) {
	if(handle == Process::noHandle) {
		return;
	}

	switch(signal) {
	case Type::hangUp:
		kill(handle, SIGHUP);
		break;
	case Type::interrupt:
		kill(handle, SIGINT);
		break;
	case Type::quit:
		kill(handle, SIGQUIT);
		break;
	case Type::ill:
		kill(handle, SIGILL);
		break;
	case Type::trap:
		kill(handle, SIGTRAP);
		break;
	case Type::abort:
		kill(handle, SIGABRT);
		break;
	case Type::busError:
		kill(handle, SIGBUS);
		break;
	case Type::floatingPointException:
		kill(handle, SIGFPE);
		break;
	case Type::user1:
		kill(handle, SIGUSR1);
		break;
	case Type::segmentationViolation:
		kill(handle, SIGSEGV);
		break;
	case Type::user2:
		kill(handle, SIGUSR2);
		break;
	case Type::pipe:
		kill(handle, SIGPIPE);
		break;
	case Type::alarm:
		kill(handle, SIGALRM);
		break;
	case Type::stackFault:
		kill(handle, SIGSTKFLT);
		break;
	case Type::terminate:
		kill(handle, SIGTERM);
		break;
	case Type::child:
		kill(handle, SIGCHLD);
		break;
	case Type::kill:
		kill(handle, SIGKILL);
		break;
	default:
		break;
	}
}

} /* namespace zsystem */
