
#include "logger.h"
#include "util.h"


#include <thread>
#define LOGGING_LOOP(iterations)            for (u32 x = 0; x < iterations; x++) LOG(Trace, "Log message: " << x);
void test_logger_multithreaded(const u32 count) {

    logger::register_label_for_thread("worker slave");
    logger::register_label_for_thread("worker 01");

    LOG(Debug, "Beginning test function");
    LOGGING_LOOP(count)
    DEBUG_BREAK("Testing multithreaded DEBUG_BREAK")
    LOG(Debug, "Ending test function");    
}


// ====================================================================================================================================
// CRASH HANDLING         need to catch signals related to termination to flash remaining log messages
// ====================================================================================================================================

#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <cstring>
#include <fstream>
#include <vector>
#include <algorithm>        // sort, unique
#include <system_error>
#include <signal.h> // sigaction
const std::initializer_list<int> signals = {
    SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGKILL, SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2,    // POSIX.1-1990 signals
    SIGBUS, SIGPOLL, SIGPROF, SIGSYS, SIGTRAP, SIGVTALRM, SIGXCPU, SIGXFSZ,                                             // SUSv2 + POSIX.1-2001 signals
    SIGIOT, SIGSTKFLT, SIGIO, SIGPWR,                                                                                   // Various other signals
};
std::vector<std::pair<int, struct sigaction>> g_old_sigactions;

void detach_crash_handler() {

    while(!g_old_sigactions.empty()) {
        auto const& p = g_old_sigactions.back();
        auto signal = p.first;
        auto const& oldact = p.second;
        if(0 != sigaction(signal, &oldact, nullptr))
            throw std::system_error(errno, std::system_category());
        g_old_sigactions.pop_back();
    }
}

void signal_handler(int) {

    logger::shutdown();
    detach_crash_handler();
}

void attach_crash_handler() {

    struct sigaction act;
    std::memset(&act, 0, sizeof(act));
    act.sa_handler = &signal_handler;
    sigfillset(&act.sa_mask);
    act.sa_flags = SA_RESETHAND;

    // Some signals are synonyms for each other. Some are explictly specified
    // as such, but others may just be implemented that way on specific
    // systems. So we'll remove duplicate entries here before we loop through
    // all the signal numbers.
    std::vector<int> unique_signals(signals);
    sort(begin(unique_signals), end(unique_signals));
    unique_signals.erase(unique(begin(unique_signals), end(unique_signals)), end(unique_signals));
    try {
        g_old_sigactions.reserve(unique_signals.size());
        for(auto signal : unique_signals) {
            struct sigaction oldact;
            if(0 != sigaction(signal, nullptr, &oldact))
                throw std::system_error(errno, std::system_category());
            if(oldact.sa_handler == SIG_DFL) {
                if(0 != sigaction(signal, &act, nullptr)) {
                    if(errno == EINVAL)             // If we get EINVAL then we assume that the kernel does not know about this particular signal number.
                        continue;

                    throw std::system_error(errno, std::system_category());
                }
                g_old_sigactions.push_back({signal, oldact});
            }
        }
    } catch(...) {
        detach_crash_handler();
        throw;
    }
}

// ====================================================================================================================================
// CRASH HANDLING         need to catch signals related to termination to flash remaining log messages
// ====================================================================================================================================

int main (int argc, char* argv[]) {

    u16 enabled_options = 0;

    LOG(Trace, "Trace log message");

    attach_crash_handler();
    logger::init("[$B$T:$J  $L$X  $A  $F:$G$E] $C$Z", true);

    if (argc == 1) {
        LOG(Info, "No specivic arguments detected. Prociging with simple demo")
        enabled_options |= BIT(0);
        LOG_SEPERATOR
    } else {

        for (int x = 1; x < argc; x++) {            
            if (argv[x][0] != '-')                                  // Not an option because it doesnt start with '-'
                continue;

            for (int y = 0; argv[x][y] != '\0'; y++) {
                switch (argv[x][y]) {                               // register arguments
                    case 's': enabled_options |= BIT(0); break;
                    case 'm': enabled_options |= BIT(1); break;
                    default: break;
                }
            }
        }
    }

    if (GET_BIT(enabled_options, 0)) {

        int test_int = 42;
        LOG(Trace, "Trace log message");
        LOG(Debug, "Debug log message with var: " << test_int);
        LOG(Info, "Info log message with var: " << test_int);
        LOG(Warn, "Warn log message with var: " << test_int);
        LOG(Error, "Error log message with var: " << test_int);
        LOG(Fatal, "Fatal log message with var: " << test_int);

        LOG_SEPERATOR
        LOG(Trace, "Testing VALIDATE() macro");
        VALIDATE(test_int == 42, , "VALIDATE (test_int == 42) correct", "VALIDATE false")
        VALIDATE(test_int == 0, , "VALIDATE (test_int == 0) correct", "VALIDATE (test_int == 0) false , no aditional commands")
        VALIDATE(test_int == 0, test_int = 0, "VALIDATE (test_int == 0) correct", "VALIDATE (test_int == 0) false , setting [test_int] to 0")

        LOG_SEPERATOR
        LOG(Trace, "Testing VALIDATE_S() macroValue of test_int: " << test_int)
        VALIDATE_S(test_int == 0, )                                                     // will pass silently
        VALIDATE_S(test_int == 42, )                                                    // will log the boolean expression in a warning message
        // int debug_var = 12;
        // DEBUG_BREAK("An error accurt. var: " << debug_var)          // disabled to continue testing

        LOG_SEPERATOR
        LOG(Trace, "Testing ASSERT() macroValue of test_int: " << test_int)
        ASSERT(test_int == 0, "ASSERT(test_int == 0): correct", "assert 0: FALSE")
        // ASSERT(test_int == 42, "assert 0: correct", "assert 0: FALSE")
    }

    if (GET_BIT(enabled_options, 1)) {

        LOG_SEPERATOR
        LOG(Trace, "Testing multithreaded logging")
        logger::set_format("[$B$T:$J  $L$X  $Q  $A  $F:$G$E] $C$Z");
        logger::register_label_for_thread("main");

        std::thread simple_worker_thread = std::thread(&test_logger_multithreaded, 10);
        LOGGING_LOOP(10)
        simple_worker_thread.join();
    }

    logger::shutdown();
    detach_crash_handler();
    return 0;
}

// clear && g++ -o logging_test main.cpp logger.cpp util.cpp -Wall -Wextra
// ./logging_test -sm
