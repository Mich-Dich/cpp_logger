
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


int main (int argc, char* argv[]) {

    u16 enabled_options = 0;
    logger::init("[$B$T:$J  $L$X  $A  $F:$G$E] $C$Z");

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
    return 0;
}

// clear && g++ -o logging_test main.cpp logger.cpp util.cpp
// ./logging_test -sm
