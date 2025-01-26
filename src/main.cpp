
#include "logger.h"
#include "util.h"
#include <iostream>

#include <thread>
#define LOGGING_LOOP(iterations)            for (u32 x = 0; x < iterations; x++) { LOG(Trace, "Log message: " << x); std::this_thread::sleep_for(std::chrono::nanoseconds(100)); }
void multithreaded_func(const u32 count) {

    logger::register_label_for_thread("worker slave");
    logger::register_label_for_thread("worker 01");         // redefenition just to test error handeling

    LOGGING_LOOP(count)
}


// ====================================================================================================================================
// CRASH HANDLING         need to catch signals related to termination to flash remaining log messages. Was inspired by reckless_log: https://github.com/mattiasflodin/reckless
// ====================================================================================================================================

#include <cstring>
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
// CRASH HANDLING
// ====================================================================================================================================

enum class options : u8 {
    simple = 0,
    multithread,
    file_dialog,
    error_provoking,
};

int main (int argc, char* argv[]) {

    u16 enabled_options = 0;
    if (argc == 1) {
        std::cout << "No specivic arguments detected. Prociging with simple demo" << std::endl;
        enabled_options |= BIT(0);
    } else {

        for (int x = 1; x < argc; x++) {            
            if (argv[x][0] != '-')                                  // Not an option because it doesnt start with '-'
                continue;

            for (int y = 0; argv[x][y] != '\0'; y++) {
                switch (argv[x][y]) {                               // register arguments
                    case 's': enabled_options |= BIT(options::simple); break;
                    case 'm': enabled_options |= BIT(options::multithread); break;
                    case 'd': enabled_options |= BIT(options::file_dialog); break;
                    case 'e': enabled_options |= BIT(options::error_provoking); break;             // error provoking, loging message bevor initalizing logger
                    default: break;
                }
            }
        }
    }

    if (GET_BIT(enabled_options, options::error_provoking))                                // logging a message as error
        LOG(Trace, "Trace log message");

    attach_crash_handler();
    logger::init("[$B$T:$J  $L$X  $I $F:$G$E] $C$Z");

    if (GET_BIT(enabled_options, options::file_dialog)) {
        // Example usage
        std::filesystem::path selectedFile = util::file_dialog("Select a file");
        if (!selectedFile.empty())
            LOG(Trace, "Selected file: " << selectedFile)
        else
            LOG(Trace, "No file selected.")

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (GET_BIT(enabled_options, options::simple)) {

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
        // DEBUG_BREAK("An error accurt. var: " << debug_var)                           // disabled to continue testing

        LOG_SEPERATOR
        LOG(Trace, "Testing ASSERT() macroValue of test_int: " << test_int)
        ASSERT(test_int == 0, "ASSERT(test_int == 0): correct", "assert 0: FALSE")
        // ASSERT(test_int == 42, "assert 0: correct", "assert 0: FALSE")
    }

    if (GET_BIT(enabled_options, options::multithread)) {

        logger::register_label_for_thread("main");
        logger::set_format("[$B$T:$J  $L$X  $Q  $I $F:$G$E] $C$Z");
        LOG_SEPERATOR
        LOG(Trace, "Testing multithreaded logging")

        u32 count = 50000;
        std::thread simple_worker_thread = std::thread(&multithreaded_func, count);
        LOGGING_LOOP(count)
        simple_worker_thread.join();
        LOG_SEPERATOR
    }

    logger::shutdown();
    detach_crash_handler();
    return 0;
}

// sudo apt install qt5-default qttools5-dev-tools
// clear && cppcheck --std=c++20 --enable=all .
