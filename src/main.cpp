
#include "logger.h"
#include "util.h"

#include <chrono>
#include <thread>

int main (int argc, char* argv[]) {

    int test_int = 42;

    logger::init("[$B$T:$J$E - $B$A $F:$G$E] $C$Z");

    LOG(Trace, "Trace log message");
    LOG(Debug, "Debug log message var: " << test_int);
    LOG(Info, "Info log message var: " << test_int);
    LOG(Warn, "Warn log message var: " << test_int);
    LOG(Error, "Error log message var: " << test_int);
    LOG(Fatal, "Fatal log message var: " << test_int);

    VALIDATE(0 == 42, , "validation 0: correct", "validation 0: false")
    VALIDATE(42 == 42, , "validation 0: correct", "validation 0: false")
    VALIDATE(42.0f == 42, , "validation 0: correct", "validation 0: false")

    VALIDATE_S(0 == 42, )
    VALIDATE_S(42 == 42, )
    VALIDATE_S(42.0f == 42, )

    VALIDATE_S(0 == 42, )

    std::this_thread::sleep_for(std::chrono::seconds(1));

    int debug_var = 12;
    DEBUG_BREAK("An error accurt. var: " << debug_var)
    VALIDATE_S(0 == 42, )

    // ASSERT(42 == 42, "assert 0: correct", "assert 0: FALSE")
    // ASSERT(0 == 42, "correct", "FALSE")                 // disabled to continue testing

    logger::shutdown();

    return 0;
}
