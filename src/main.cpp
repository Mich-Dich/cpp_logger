
#include "logger.h"
#include "util.h"


#include <thread>
#define ITERATIONS              10
void test_logger_multithreaded(const u32 count) {

    LOG(Debug, "Beginning test function");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    logger::register_label_for_thread("worker 01");

    for (u32 x = 0; x < count; x++) {
        
        LOG(Trace, "Log message: " << x);
    }
    LOG(Debug, "Ending test function");    
}


int main (int argc, char* argv[]) {

    logger::init("[$B$T:$J  $L$X  $A  $Q  $F:$G$E] $C$Z");
    logger::register_label_for_thread("main");

    std::thread simple_worker_thread = std::thread(&test_logger_multithreaded, ITERATIONS);
    test_logger_multithreaded(ITERATIONS);

    simple_worker_thread.join();

    logger::shutdown();
    return 0;
}


// LOG(Trace, "Trace log message");
// LOG(Debug, "Debug log message var: " << test_int);
// LOG(Info, "Info log message var: " << test_int);
// LOG(Warn, "Warn log message var: " << test_int);
// LOG(Error, "Error log message var: " << test_int);
// LOG(Fatal, "Fatal log message var: " << test_int);

// VALIDATE(0 == 42, , "validation 0: correct", "validation 0: false")
// VALIDATE(42 == 42, , "validation 0: correct", "validation 0: false")
// VALIDATE(42.0f == 42, , "validation 0: correct", "validation 0: false")

// VALIDATE_S(0 == 42, )
// VALIDATE_S(42 == 42, )
// VALIDATE_S(42.0f == 42, )

// VALIDATE_S(0 == 42, )

// // int debug_var = 12;
// // DEBUG_BREAK("An error accurt. var: " << debug_var)          // disabled to continue testing
// VALIDATE_S(0 == 42, )

// ASSERT(42 == 42, "assert 0: correct", "assert 0: FALSE")