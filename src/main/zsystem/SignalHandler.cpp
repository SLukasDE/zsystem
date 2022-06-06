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

#include <zsystem/SignalHandler.h>

#include <list>
#include <map>
#include <algorithm>
#include <exception>

#include <string.h>
#include <errno.h>
#include <signal.h>

namespace zsystem {

namespace {

struct InstalledSignalHandlers {
	InstalledSignalHandlers(int signalNumber)
	: signalNumber(signalNumber)
	{ }

	const int signalNumber;
	std::list<std::unique_ptr<SignalHandler>> handlers;
	struct sigaction sigActionInstalled;
	struct sigaction sigActionOriginal;
};

InstalledSignalHandlers installedSignalHandlersHangUp(SIGHUP);                 // 01
InstalledSignalHandlers installedSignalHandlersInterrupt(SIGINT);              // 02
InstalledSignalHandlers installedSignalHandlersQuit(SIGQUIT);                  // 03
InstalledSignalHandlers installedSignalHandlersIll(SIGILL);                    // 04
InstalledSignalHandlers installedSignalHandlersTrap(SIGTRAP);                  // 05
InstalledSignalHandlers installedSignalHandlersAbort(SIGABRT);                 // 06
InstalledSignalHandlers installedSignalHandlersBusError(SIGBUS);               // 07
InstalledSignalHandlers installedSignalHandlersFloatingPointException(SIGFPE); // 08
InstalledSignalHandlers installedSignalHandlersUser1(SIGUSR1);                 // 10
InstalledSignalHandlers installedSignalHandlersSegmentationViolation(SIGSEGV); // 11
InstalledSignalHandlers installedSignalHandlersUser2(SIGUSR2);                 // 12
InstalledSignalHandlers installedSignalHandlersPipe(SIGPIPE);                  // 13
InstalledSignalHandlers installedSignalHandlersAlarm(SIGALRM);                 // 14
InstalledSignalHandlers installedSignalHandlersTerminate(SIGTERM);             // 15
InstalledSignalHandlers installedSignalHandlersStackFault(SIGSTKFLT);          // 16
InstalledSignalHandlers installedSignalHandlersChild(SIGCHLD);                 // 17

std::map<Signal::Type, InstalledSignalHandlers*> installedSignalHandlersByType = {
		{Signal::Type::hangUp,                 &installedSignalHandlersHangUp},
		{Signal::Type::interrupt,              &installedSignalHandlersInterrupt},
		{Signal::Type::quit,                   &installedSignalHandlersQuit},
		{Signal::Type::ill,                    &installedSignalHandlersIll},
		{Signal::Type::trap,                   &installedSignalHandlersTrap},
		{Signal::Type::abort,                  &installedSignalHandlersAbort},
		{Signal::Type::busError,               &installedSignalHandlersBusError},
		{Signal::Type::floatingPointException, &installedSignalHandlersFloatingPointException},
		{Signal::Type::user1,                  &installedSignalHandlersUser1},
		{Signal::Type::segmentationViolation,  &installedSignalHandlersSegmentationViolation},
		{Signal::Type::user2,                  &installedSignalHandlersUser2},
		{Signal::Type::pipe,                   &installedSignalHandlersPipe},
		{Signal::Type::alarm,                  &installedSignalHandlersAlarm},
		{Signal::Type::stackFault,             &installedSignalHandlersStackFault},
		{Signal::Type::terminate,              &installedSignalHandlersTerminate},
		{Signal::Type::child,                  &installedSignalHandlersIll}
};

InstalledSignalHandlers* signalTypeToInstalledSignalHandlers(Signal::Type signalType) {
	auto iter = installedSignalHandlersByType.find(signalType);
	return (iter == std::end(installedSignalHandlersByType)) ? nullptr : iter->second;
}

} /* namespace {anonymous} */

SignalHandler::Handle::Handle(SignalHandler& aSignalHandler)
: signalHandler(&aSignalHandler)
{ }

SignalHandler::Handle::Handle(SignalHandler::Handle&& handle)
: signalHandler(handle.signalHandler)
{
	handle.signalHandler = nullptr;
}

SignalHandler::Handle::~Handle() {
	if(signalHandler) {
		SignalHandler::remove(*signalHandler);
	}
}

SignalHandler::Handle& SignalHandler::Handle::operator=(SignalHandler::Handle&& handle) {
	signalHandler = handle.signalHandler;
	handle.signalHandler = nullptr;

	return *this;
}

SignalHandler::SignalHandler(Signal::Type signalType, std::function<void()> aHandler)
: handler(aHandler),
  type(signalType)
{ }

SignalHandler::Handle SignalHandler::install(Signal::Type signalType, std::function<void()> handler) {
	InstalledSignalHandlers* installedSignalHandlers = signalTypeToInstalledSignalHandlers(signalType);

	if(installedSignalHandlers == nullptr) {
        throw std::runtime_error("Cannot install signal handler for unknown signal type");
    }

    if(installedSignalHandlers->handlers.empty()) {
    	installedSignalHandlers->sigActionInstalled.sa_handler = saHandler;
        sigemptyset(&installedSignalHandlers->sigActionInstalled.sa_mask);

#ifdef SA_INTERRUPT
        installedSignalHandlers->sigActionInstalled.sa_flags = SA_INTERRUPT;
#else
        installedSignalHandlers->sigActionInstalled.sa_flags = SA_RESTART;
#endif
        bool failure = (sigaction(installedSignalHandlers->signalNumber, &installedSignalHandlers->sigActionInstalled, &installedSignalHandlers->sigActionOriginal) != 0);
        if(failure) {
            throw std::runtime_error("Installation of signal handler failed for signal number " + std::to_string(installedSignalHandlers->signalNumber));
        }
    }

    std::unique_ptr<SignalHandler> signalHandler(new SignalHandler(signalType, handler));
    SignalHandler::Handle handle(*signalHandler);

    installedSignalHandlers->handlers.push_back(std::move(signalHandler));

    return handle;
}

void SignalHandler::remove(SignalHandler& signalHandler) {
//void SignalHandler::remove(Signal::Type signalType, std::function<void()> handler) {
//	InstalledSignalHandlers* installedSignalHandlers = signalTypeToInstalledSignalHandlers(signalType);
	InstalledSignalHandlers* installedSignalHandlers = signalTypeToInstalledSignalHandlers(signalHandler.type);

    if(installedSignalHandlers) {
    	std::list<std::unique_ptr<SignalHandler>>::iterator iterHandlers = installedSignalHandlers->handlers.begin();
    	while(iterHandlers != installedSignalHandlers->handlers.end()) {
    		if(iterHandlers->get() != &signalHandler) {
        		++iterHandlers;
        		continue;
    		}

			iterHandlers = installedSignalHandlers->handlers.erase(iterHandlers);

	        if(installedSignalHandlers->handlers.empty()) {
	            sigaction(installedSignalHandlers->signalNumber, &installedSignalHandlers->sigActionOriginal, nullptr);
	        }
    	}
    }
}

void SignalHandler::saHandler(int signalNumber) {
	auto iter = std::find_if(std::begin(installedSignalHandlersByType), std::end(installedSignalHandlersByType),
			[signalNumber](const std::pair<Signal::Type, InstalledSignalHandlers*>& e) {
		return e.second->signalNumber == signalNumber;
	});

	if(iter == std::end(installedSignalHandlersByType)) {
    	// ERROR! unknown signal number. Who installed this handler?
        // esl::logger.error << "Signal number " << signalNumber << " is not supported by this framework. Who installed this handler?" << std::endl;
        return;
	}

    InstalledSignalHandlers& installedSignalHandlers = *iter->second;

    if(installedSignalHandlers.handlers.empty()) {
    	// WARNING! Empty handler list should result in installing the original handler again?!
    }

    for(const auto& signalHandler : installedSignalHandlers.handlers) {
        signalHandler->handler();
    }
}

} /* namespace zsystem */

