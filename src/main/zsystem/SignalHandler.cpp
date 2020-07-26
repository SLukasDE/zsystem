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

#include <zsystem/SignalHandler.h>

#include <list>
#include <map>
#include <algorithm>
#include <functional>
#include <exception>
#include <string.h>
#include <errno.h>
#include <signal.h>

namespace zsystem {

namespace {

class LambdaSignalHandler : public SignalHandler {
public:
    LambdaSignalHandler(Type signalType, std::function<void()> handler)
    : SignalHandler(signalType),
      handler(handler)
    {
    }

    std::function<void()> handler;

protected:
    void invoke() override {
    	handler();
    }
};

struct InstalledSignalHandlers {
	InstalledSignalHandlers(int signalNumber)
	: signalNumber(signalNumber)
	{
	}

	const int signalNumber;
	bool failure = false;
	std::list<std::reference_wrapper<SignalHandler>> handlers;
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

std::map<SignalHandler::Type, InstalledSignalHandlers> installedSignalHandlersByType = {
		{SignalHandler::Type::hangUp,                 installedSignalHandlersHangUp},
		{SignalHandler::Type::interrupt,              installedSignalHandlersInterrupt},
		{SignalHandler::Type::quit,                   installedSignalHandlersQuit},
		{SignalHandler::Type::ill,                    installedSignalHandlersIll},
		{SignalHandler::Type::trap,                   installedSignalHandlersTrap},
		{SignalHandler::Type::abort,                  installedSignalHandlersAbort},
		{SignalHandler::Type::busError,               installedSignalHandlersBusError},
		{SignalHandler::Type::floatingPointException, installedSignalHandlersFloatingPointException},
		{SignalHandler::Type::user1,                  installedSignalHandlersUser1},
		{SignalHandler::Type::segmentationViolation,  installedSignalHandlersSegmentationViolation},
		{SignalHandler::Type::user2,                  installedSignalHandlersUser2},
		{SignalHandler::Type::pipe,                   installedSignalHandlersPipe},
		{SignalHandler::Type::alarm,                  installedSignalHandlersAlarm},
		{SignalHandler::Type::stackFault,             installedSignalHandlersStackFault},
		{SignalHandler::Type::terminate,              installedSignalHandlersTerminate},
		{SignalHandler::Type::child,                  installedSignalHandlersIll}
};

InstalledSignalHandlers* signalTypeToInstalledSignalHandlers(SignalHandler::Type signalType) {
	auto iter = installedSignalHandlersByType.find(signalType);
	return (iter == std::end(installedSignalHandlersByType)) ? nullptr : &iter->second;
}

InstalledSignalHandlers* signalNumberToInstalledSignalHandlers(int signalNumber) {
	auto iter = std::find_if(std::begin(installedSignalHandlersByType), std::end(installedSignalHandlersByType),
			[signalNumber](const std::pair<SignalHandler::Type, InstalledSignalHandlers>& e) {
		return e.second.signalNumber == signalNumber;
	});
	return (iter == std::end(installedSignalHandlersByType)) ? nullptr : &iter->second;
}

void signalHandler(int signalNumber) {
	auto iter = std::find_if(std::begin(installedSignalHandlersByType), std::end(installedSignalHandlersByType),
			[signalNumber](const std::pair<SignalHandler::Type, InstalledSignalHandlers>& e) {
		return e.second.signalNumber == signalNumber;
	});

	if(iter == std::end(installedSignalHandlersByType)) {
    	// ERROR! unknown signal number. Who installed this handler?
        // esl::logger.error << "Signal number " << signalNumber << " is not supported by this framework. Who installed this handler?" << std::endl;
        return;
	}

    SignalHandler::handle(iter->first);
}

} /* namespace {anonymous} */

SignalHandler::SignalHandler(Type signalType)
: type(signalType)
{
	InstalledSignalHandlers* installedSignalHandlers = signalTypeToInstalledSignalHandlers(signalType);

	if(installedSignalHandlers == nullptr) {
        throw std::runtime_error("Cannot install signal handler for unknown signal type");
        return;
    }

	if(installedSignalHandlers->failure) {
        throw std::runtime_error("Cannot install signal handler for signal " + std::to_string(installedSignalHandlers->signalNumber) + " because of previous failure.");
        return;
    }

    if(installedSignalHandlers->handlers.empty()) {
    	installedSignalHandlers->sigActionInstalled.sa_handler = signalHandler;
        sigemptyset(&installedSignalHandlers->sigActionInstalled.sa_mask);

#ifdef SA_INTERRUPT
        installedSignalHandlers->sigActionInstalled.sa_flags = SA_INTERRUPT;
#else
        installedSignalHandlers->sigActionInstalled.sa_flags = SA_RESTART;
#endif
        installedSignalHandlers->failure = (sigaction(installedSignalHandlers->signalNumber, &installedSignalHandlers->sigActionInstalled, &installedSignalHandlers->sigActionOriginal) != 0);
        if(installedSignalHandlers->failure) {
            throw std::runtime_error("Installation of signal handler failed for signal number " + std::to_string(installedSignalHandlers->signalNumber));
        }
    }

    installedSignalHandlers->handlers.push_back(std::ref(*this));
}

SignalHandler::~SignalHandler() {
    InstalledSignalHandlers* installedSignalHandlers = signalTypeToInstalledSignalHandlers(type);

    if(installedSignalHandlers) {
    	installedSignalHandlers->handlers.remove_if([this](const std::reference_wrapper<SignalHandler>& handler){return &handler.get() == this;});

        if(installedSignalHandlers->handlers.empty()) {
            sigaction(installedSignalHandlers->signalNumber, &installedSignalHandlers->sigActionOriginal, nullptr);
        }
    }
}

void SignalHandler::install(Type signalType, std::function<void()> handler) {
	new LambdaSignalHandler(signalType, handler);
}

void SignalHandler::remove(Type signalType, std::function<void()> handler) {
	InstalledSignalHandlers* installedSignalHandlers = signalTypeToInstalledSignalHandlers(signalType);
	for(auto signalHandlerObj : installedSignalHandlers->handlers) {
		LambdaSignalHandler* ptr = dynamic_cast<LambdaSignalHandler*>(&signalHandlerObj.get());
		if(ptr && ptr->handler.target<void()>() == handler.target<void()>()) {
			delete ptr;
			return;
		}
	}
}

void SignalHandler::handle(SignalHandler::Type signalType) {
    InstalledSignalHandlers* installedSignalHandlers = signalTypeToInstalledSignalHandlers(signalType);

    if(installedSignalHandlers == nullptr) {
    	// ERROR! unknown signal number. Who installed this handler?
        // esl::logger.error << "Signal type " << signalType << " is not supported by this framework. Who installed this handler?" << std::endl;
        return;
    }

    if(installedSignalHandlers->handlers.empty()) {
    	// WARNING! Empty handler list should result in installing the original handler again?!
    }

    for(auto signalHandler : installedSignalHandlers->handlers) {
        signalHandler.get().invoke();
    }
}

} /* namespace zsystem */

