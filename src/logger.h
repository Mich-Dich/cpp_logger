
#include <string>
#include <filesystem>
#include <exception>

#include "util.h"

namespace logger {

    enum class severity : u8 {
        Trace = 0,
        Debug,
        Info,
        Warn,
        Error,
        Fatal,
    };

    struct message_format {

        message_format(const logger::severity msg_sev, const char* file_name, const char* function_name, const int line, const std::string& message) 
            : msg_sev(msg_sev), file_name(file_name), function_name(function_name), line(line), message(message) {};

        const logger::severity  msg_sev;
        const char*             file_name;
        const char*             function_name;
        const int               line;
        const std::string       message;
    };

    // Initalize the logging system
    // @param format The iital log message foemat
    // @param log_dir the directory that will contain all log files
    // @ main_log_file_name name of the central log_file (the thread that runs logger::init())
    // @param use_append_mode Should the system write over the existing log file or append to it
    bool init(const std::string& format, const std::filesystem::path log_dir = "./logs", const std::string& main_log_file_name = "general.log", const bool use_append_mode = false);

    // shutdown the logging system
    void shutdown();

    // The format of log-messages can be custimized with the following tags
    // @note to format all following log-messages use: set_format()
    // @note e.g. set_format("$B[$T] $L [$F] $C$E")
    //
    // @param $T time                    hh:mm:ss
    // @param $H hour                    hh
    // @param $H minute                  mm
    // @param $S secunde                 ss
    // @param $J millisecunde            jjj
    //      
    // @param $N data                    yyyy:mm:dd
    // @param $Y data year               yyyy
    // @param $O data month              mm
    // @param $D data day                dd
    //
    // @param $F function name           application::main, math::foo
    // @param $P only function name      main, foo
    // @param $A file name               C:/project/main.cpp  ~/project/main.cpp
    // @param $K shortend file name      project/main.cpp
    // @param $I only file name          main.cpp
    // @param $G line                    1, 42
    //
    // @param $L log-level               add used log severity: [TRACE], [DEBUG] ... [FATAL]
    // @param $X alignment               adds space for "INFO" & "WARN"
    // @param $B color begin             from here the color begins
    // @param $E color end               from here the color will be reset
    // @param $C text                    the message the user wants to print
    // @param $Z new line                add a new line in the message format
    void set_format(const std::string& new_format);

    // Restore the previous log-message format
    // @note This function swaps the current log-message format with the previously stored backup.
    // It's useful for revertinng to the previous format after temporary changes
    void use_previous_format();

    // get the currently used log-message format
    const std::string get_format();

    // define with messages severitys should be written to log directly and witch can be buffered
    // @param sev_level 0 => write all logs directly to file | 1 => buffer: TRACE | 2 => buffer: TRACE + DEBUG | 3 => buffer: TRACE + DEBUG + INFO | 4 => buffer: TRACE + DEBUG + INFO + WARN
    // @param new_max_size_of_buffer set the number of messages that can be buffered bevor wrinting to file
    void set_buffer_settings(const severity sev_level, const u32 new_max_size_of_buffer);

    // // THIS SHOULD NEVER BE DIRECTLY CALLED
    // // @note empty log messages will be ignored
    void log_msg(const severity msg_sev , const char* file_name, const char* function_name, const int line, std::string message );
}




class debug_break_exception : public std::exception {
public:
    explicit debug_break_exception(const std::string& message)
        : m_msg(message) {}

    virtual const char* what() const noexcept override { return m_msg.c_str(); }

private:
    std::string m_msg;
};

#define DEBUG_BREAK(message)                { std::ostringstream oss; oss << "DEBUG BREAK [file: " __FILE__ << ", function: " << __FUNCTION__ << ", line: " << __LINE__ << "] => "<< message; throw debug_break_exception(oss.str()); }



// This define enables the diffrent log levels (FATAL & ERROR are always on)
//  0 => FATAL + ERROR
//  1 => FATAL + ERROR + WARN
//  2 => FATAL + ERROR + WARN + INFO
//  3 => FATAL + ERROR + WARN + INFO + DEBUG
//  4 => FATAL + ERROR + WARN + INFO + DEBUG + TRACE
#define LOG_LEVEL_ENABLED                   4

#define ENABLED_LOGGING_OF_ASSERTS          1
#define ENABLE_LOGGING_OF_VALIDATION        1

//  =================================================================================== Logger  ===================================================================================

// always enabled
#define LOG_Error(message)                  { std::ostringstream oss; oss << message; logger::log_msg(logger::severity::Fatal, __FILE__, __FUNCTION__, __LINE__, std::move(oss.str())); }
#define LOG_Fatal(message)                  { std::ostringstream oss; oss << message; logger::log_msg(logger::severity::Error, __FILE__, __FUNCTION__, __LINE__, std::move(oss.str())); }

#if LOG_LEVEL_ENABLED > 0
    #define LOG_Warn(message)               { std::ostringstream oss; oss << message; logger::log_msg(logger::severity::Warn, __FILE__, __FUNCTION__, __LINE__, std::move(oss.str())); }
#else
    #define LOG_Warn(message)
#endif

#if LOG_LEVEL_ENABLED > 1
    #define LOG_Info(message)               { std::ostringstream oss; oss << message; logger::log_msg(logger::severity::Info, __FILE__, __FUNCTION__, __LINE__, std::move(oss.str())); }
#else
    #define LOG_Info(message)
#endif

#if LOG_LEVEL_ENABLED > 2
    #define LOG_Debug(message)              { std::ostringstream oss; oss << message; logger::log_msg(logger::severity::Debug, __FILE__, __FUNCTION__, __LINE__, std::move(oss.str())); }
#else
    #define LOG_Debug(message)
#endif

#if LOG_LEVEL_ENABLED > 3
    #define LOG_Trace(message)              { std::ostringstream oss; oss << message; logger::log_msg(logger::severity::Trace, __FILE__, __FUNCTION__, __LINE__, std::move(oss.str())); }
#else
    #define LOG_Trace(message)
#endif

#define LOG(severity, message)              LOG_##severity(message)

// ---------------------------------------------------------------------------  Assertion & Validation  ---------------------------------------------------------------------------

#if ENABLED_LOGGING_OF_ASSERTS
    #define ASSERT(expr, successMsg, failureMsg)							\
                    if (expr) 												\
                        LOG(Trace, successMsg)							    \
                    else {												    \
                        LOG(Fatal, failureMsg)						        \
                        DEBUG_BREAK(failureMsg);							\
                    }

    #define ASSERT_S(expr)													\
                    if (!(expr)) {									        \
                        LOG(Fatal, #expr)						            \
                        DEBUG_BREAK(#expr);     					        \
                    }
#else
    #define ASSERT(expr, successMsg, failureMsg)							if (!(expr)) { DEBUG_BREAK(failureMsg); }
    #define ASSERT_S(expr)													if (!(expr)) { DEBUG_BREAK(#expr); }
#endif // ENABLED_LOGGING_OF_ASSERT


#if ENABLE_LOGGING_OF_VALIDATION
    #define VALIDATE(expr, ReturnCommand, successMsg, failureMsg)			\
                    if (expr) 												\
                        LOG(Trace, successMsg)							    \
                    else {													\
                        LOG(Warn, failureMsg)							    \
                        ReturnCommand;										\
                    }

    #define VALIDATE_S(expr, ReturnCommand)									\
                    if (!(expr)) {											\
                        LOG(Warn, "Validation Failed: " << #expr)		    \
                        ReturnCommand;										\
                    }
#else
    #define VALIDATE(expr, ReturnCommand, successMsg, failureMsg)			if (!(expr)) { ReturnCommand; }
    #define VALIDATE_S(expr, ReturnCommand)						            if (!(expr)) { ReturnCommand; }

#endif // ENABLE_LOGGING_OF_VALIDATION
