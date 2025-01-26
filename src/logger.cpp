
#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <string>
#include <string_view>
#include <cstring>
#include <fstream>

#if defined __WIN32__
    #include <Windows.h>
#elif defined __unix__
    #include <time.h>
    #include <sys/time.h>
    #include <ctime>
#endif

#include "util.h"
#include "logger.h"


#define TIME_FORMATTER_PERFORMANCE
#ifdef TIME_FORMATTER_PERFORMANCE
    u32 formatting_counter = 0;
    f32 cumulative_formatting_duration = 0;
#endif


#define TIME_LOGGING_PERFORMANCE
#ifdef TIME_LOGGING_PERFORMANCE
    u32 logging_counter = 0;
    f32 cumulative_logging_duration = 0;
#endif


#define TIME_COUT_PERFORMANCE
#ifdef TIME_COUT_PERFORMANCE
    u32 cout_counter = 0;
    f32 cumulative_cout_duration = 0;
#endif


#define TIME_QUEUE_ADDING_PERFORMANCE
#ifdef TIME_QUEUE_ADDING_PERFORMANCE
    u32 queue_adding_counter = 0;
    f32 cumulative_queue_adding_duration = 0;
#endif


#define TIME_WRITING_TO_FILE_PERFORMANCE
#ifdef TIME_WRITING_TO_FILE_PERFORMANCE
    u32 writing_to_file_counter = 0;
    f32 cumulative_writing_to_file_duration = 0;
#endif


namespace logger {

#define SETW(width)                                             std::setw(width) << std::setfill('0')
#define SHORT_FILE(text)                                        (strrchr(text, "\\") ? strrchr(text, "\\") + 1 : text)
#define SHORTEN_FUNC_NAME(text)                                 (strstr(text, "::") ? strstr(text, "::") + 2 : text)

#define LOGGER_UPDATE_FORMAT                                    "LOGGER update format"
#define LOGGER_REVERSE_FORMAT                                   "LOGGER reverse format"

    // const after init() and bevor shutdown()
    static bool                                                 is_init = false;
    static bool                                                 write_logs_to_console = false;
    static std::filesystem::path                                main_log_dir = "";
    static std::filesystem::path                                main_log_file_path = "";
    static std::thread                                          worker_thread;

    // thread savety related
    static std::condition_variable                              cv{};
    static std::mutex                                           queue_mutex{};                      // only queue related
    static std::mutex                                           general_mutex{};                    // for everything else
    static std::atomic<bool>                                    ready = false;
    static std::atomic<bool>                                    stop = false;

    // always const variables
    static std::string_view                                     severity_names[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
    static std::string_view                                     console_reset = "\x1b[0m";
    static std::string_view                                     console_color_table[] = {
        "\x1b[38;5;246m",                                           // Trace: Gray
        "\x1b[94m",                                                 // Debug: Blue
        "\x1b[92m",                                                 // Info: Green
        "\x1b[33m",                                                 // Warn: Yellow
        "\x1b[31m",                                                 // Error: Red
        "\x1b[41m\x1b[30m",                                         // Fatal: Red Background
    };

    static std::string                                          format_current = "";
    static std::string                                          format_prev = "";
    static std::ofstream                                        main_file;
    static std::queue<message_format>                           log_queue{};
    static std::unordered_map<std::thread::id, std::string>     thread_lable_map = {};

    void process_queue();
    void process_log_message(const message_format&& message);
    void detach_crash_handler();

    inline const char* get_filename(const char* filepath) {

        const char* filename = std::strrchr(filepath, '\\');
        if (filename == nullptr)
            filename = std::strrchr(filepath, '/');

        if (filename == nullptr)
            return filepath;  // No path separator found, return the whole string

        return filename + 1;  // Skip the path separator
    }


#define OPEN_MAIN_FILE(append)              { if (!main_file.is_open()) {                                                                               \
                                                main_file = std::ofstream(main_log_file_path, (append) ? std::ios::app : std::ios::out);                \
                                                if (!main_file.is_open())                                                                               \
                                                    DEBUG_BREAK("FAILED to open log main_file") } }

#define CLOSE_MAIN_FILE()                   main_file.close();



    // ====================================================================================================================================
    // init / shutdown
    // ====================================================================================================================================

    bool init(const std::string& format, const bool log_to_console, const std::filesystem::path log_dir, const std::string& main_log_file_name, const bool use_append_mode) {

        if (is_init)
            DEBUG_BREAK("Tryed to init lgging system multiple times")

        format_current = format;
        format_prev = format;
        write_logs_to_console = log_to_console;

        if (!std::filesystem::is_directory(log_dir))                            // if not already created
            if (!std::filesystem::create_directory(log_dir))                    // try to create dir
                DEBUG_BREAK("FAILED to create directory for log files")

        main_log_dir = log_dir;
        main_log_file_path = log_dir / main_log_file_name;

        OPEN_MAIN_FILE(use_append_mode)

        if (use_append_mode)
            main_file << "\n=============================================================================\n";

        // add some general info to beginning of file
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        main_file << "Log initialized at [" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "]\n"
            << "Inital Log Format: '" << format << "' \nEnabled Log Levels: ";

        const char* log_sev_strings[] = { "Fatal", " + Error", " + Warn", " + Info", " + Debug", " + Trace"};
        for (int x = 0; x < LOG_LEVEL_ENABLED +2; x++)
            main_file << log_sev_strings[x];
        main_file << "\n=============================================================================\n";

        worker_thread = std::thread(&process_queue);

        is_init = true;
        return true;
    }

    void shutdown() {

        if (!is_init)
            DEBUG_BREAK("logger::shutdown() was called bevor logger was initalized")

        stop = true;
        cv.notify_all();

        if (worker_thread.joinable())
            worker_thread.join();

        if (main_file.is_open())
            CLOSE_MAIN_FILE()

#ifdef TIME_FORMATTER_PERFORMANCE
        std::cout << std::left << std::setw(40) << "[LOGGER] Formatting performance:" << " counter [" << std::setw(8) << formatting_counter << "] average time[" << cumulative_formatting_duration / formatting_counter << " micro-s]" << std::endl;
#endif
#ifdef TIME_COUT_PERFORMANCE
        std::cout << std::left << std::setw(40) << "[LOGGER] Cout performance:" << " counter [" << std::setw(8) << cout_counter << "] average time[" << cumulative_cout_duration / cout_counter << " micro-s]" << std::endl;
#endif
#ifdef TIME_QUEUE_ADDING_PERFORMANCE
        std::cout << std::left << std::setw(40) << "[LOGGER] Queue performance:" << " counter [" << std::setw(8) << queue_adding_counter << "] average time[" << cumulative_queue_adding_duration / queue_adding_counter << " micro-s]" << std::endl;
#endif
#ifdef TIME_WRITING_TO_FILE_PERFORMANCE
        std::cout << std::left << std::setw(40) << "[LOGGER] writing to file performance:" << " counter [" << std::setw(8) << writing_to_file_counter << "] average time[" << cumulative_writing_to_file_duration / writing_to_file_counter << " micro-s]" << std::endl;
#endif

        is_init = false;
    }

    // ====================================================================================================================================
    // settings
    // ====================================================================================================================================

    void set_format(const std::string& new_format) {

        // if (!is_init) {
        //
    	// 	std::cerr << "Tryed to set logger format bevor logger was initalized" << std::endl;
        //     return;
        // }

        std::lock_guard<std::mutex> lock(queue_mutex);
        log_queue.emplace(severity::Trace, "", LOGGER_UPDATE_FORMAT, 0, static_cast<std::thread::id>(0), std::move(new_format));
        cv.notify_all();
    }

    void use_previous_format() {
        
        std::lock_guard<std::mutex> lock(queue_mutex);
        log_queue.emplace(severity::Trace, "", LOGGER_REVERSE_FORMAT, 0, static_cast<std::thread::id>(0), "");
        cv.notify_all();
    }

    const std::string get_format() { return format_current; }

    void register_label_for_thread(const std::string& thread_lable, std::thread::id thread_id) {

        std::lock_guard<std::mutex> lock(general_mutex);

        if (thread_lable_map.find(thread_id) != thread_lable_map.end())
            main_file << "[LOGGER] Thread with ID: [" << thread_id << "] already has lable [" << thread_lable_map[thread_id] << "] registered. Overriding with the lable: [" << thread_lable << "]\n";
        else
            main_file << "[LOGGER] Registering Thread-ID: [" << thread_id << "] with the lable: [" << thread_lable << "]\n";

        thread_lable_map[thread_id] = thread_lable;
    }

    void unregister_label_for_thread(std::thread::id thread_id) {

        if (thread_lable_map.find(thread_id) == thread_lable_map.end()) {

            std::lock_guard<std::mutex> lock(general_mutex);
            main_file << "[LOGGER] Tried to unregister lable for Thread-ID: [" << thread_id << "]. IGNORED\n";
            return;
        }

        std::lock_guard<std::mutex> lock(general_mutex);
        main_file << "[LOGGER] Unregistering Thread-ID: [" << thread_id << "] with the lable: [" << thread_lable_map[thread_id] << "]\n";

        thread_lable_map.erase(thread_id);
    }

    // ====================================================================================================================================
    // log message handeling
    // ====================================================================================================================================

    void process_update_in_msg_format(const message_format msg_format) {

        std::lock_guard<std::mutex> lock(general_mutex);
        format_prev = format_current;
        format_current = msg_format.message;
        main_file << "[LOGGER] Changing log-format. From [" << format_prev << "] to [" << format_current << "]\n";
    }

    void process_reverse_in_msg_format() {

        std::lock_guard<std::mutex> lock(general_mutex);
        const std::string buffer = format_current;
        format_current = format_prev;
        format_prev = buffer;
        main_file << "[LOGGER] Changing to previous log-format. From [" << format_prev << "] to [" << format_current << "]\n";
    }

    void process_queue() {

        while (!stop || !log_queue.empty()) { // Continue until stop is true and the queue is empty

            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [] { return !log_queue.empty(); });

            while (!log_queue.empty()) {

                // get message from queue
                if (!lock.owns_lock())
                    lock.lock();
                message_format message = std::move(log_queue.front());
                log_queue.pop();
                lock.unlock();

                if (strcmp(message.function_name, LOGGER_UPDATE_FORMAT) == 0)
                    process_update_in_msg_format(std::move(message));                   // change logger format from this point onwarts
                else if(strcmp(message.function_name, LOGGER_REVERSE_FORMAT) == 0)
                    process_reverse_in_msg_format();                                    // use last used format from this point onwarts
                else
                    process_log_message(std::move(message));                            // Process the message (format and write to file)
            }

            if (lock.owns_lock())                               // savety check
                lock.unlock();
        }
    }

    void log_msg(const severity msg_sev , const char* file_name, const char* function_name, const int line, const std::thread::id thread_id, const std::string& message) {

#ifdef TIME_QUEUE_ADDING_PERFORMANCE
        f32 queue_adding_duration = 0;
        util::stopwatch queue_adding_stopwatch = util::stopwatch(&queue_adding_duration, util::duration_precision::microseconds);
#endif

        if (message.empty())
            return;                      // dont log empty lines

        if (!is_init) {

    		std::cerr << "Tryed to log message bevor logger was initalized. SOURCE: file_name[" << file_name << "] function_name[" << function_name << "] line[" << line << "] thread_id[" << thread_id << "]  MESSAGE: [" << message << "] " << std::endl;
            return;
        }

        std::lock_guard<std::mutex> lock(queue_mutex);
        log_queue.emplace(msg_sev, file_name, function_name, line, thread_id, std::move(message));
        cv.notify_all();

#ifdef TIME_QUEUE_ADDING_PERFORMANCE
        queue_adding_stopwatch.stop();
        cumulative_queue_adding_duration += queue_adding_duration;
        queue_adding_counter++;
#endif
    }

    void process_log_message(const message_format&& message) {

#ifdef TIME_FORMATTER_PERFORMANCE

        // tested with std::ostringstream for [100020] iterations. average duration needed: [4.57389 micro-s]

        f32 formating_duration = 0;
        util::stopwatch formatting_stopwatch = util::stopwatch(&formating_duration, util::duration_precision::microseconds);
#endif

        // Create Buffer vars
        std::ostringstream Format_Filled;
        Format_Filled.flush();
        char Format_Command;

        util::system_time loc_system_time = util::get_system_time();

        // Loop over Format string and build Final Message
        int FormatLen = static_cast<int>(format_current.length());
        for (int x = 0; x < FormatLen; x++) {

            if (format_current[x] == '$' && x + 1 < FormatLen) {

                Format_Command = format_current[x + 1];
                switch (Format_Command) {

                // ------------------------------------  Basic Info  -------------------------------------------------------------------------------
                case 'B':   Format_Filled << console_color_table[(u8)message.msg_sev]; break;                                                                                               // Color Start
                case 'E':   Format_Filled << console_reset; break;                                                                                                                          // Color End
                case 'C':   Format_Filled << message.message; break;                                                                                                                        // input text (message)
                case 'L':   Format_Filled << severity_names[(u8)message.msg_sev]; break;                                                                                                    // Log Level
                case 'X':   if (message.msg_sev == severity::Info || message.msg_sev == severity::Warn) { Format_Filled << " "; } break;                                                    // Alignment
                case 'Z':   Format_Filled << "\n"; break;                                                                                                                                   // Alignment

                // ------------------------------------  Source  -------------------------------------------------------------------------------
                case 'Q':   if (thread_lable_map.find(message.thread_id) != thread_lable_map.end()) {Format_Filled << thread_lable_map[message.thread_id]; } else { Format_Filled << message.thread_id; } break;      // Thread id or asosiated lable
                case 'F':   Format_Filled << message.function_name; break;                                                                                                                  // Function Name
                case 'P':   Format_Filled << SHORTEN_FUNC_NAME(message.function_name); break;                                                                                               // Function Name
                case 'A':   Format_Filled << message.file_name; break;                                                                                                                      // File Name
                case 'I':   Format_Filled << get_filename(message.file_name); break;                                                                                                        // Only File Name
                case 'G':   Format_Filled << message.line; break;                                                                                                                           // Line

                // ------------------------------------  Time  -------------------------------------------------------------------------------
                case 'T':   Format_Filled << SETW(2) << (u16)loc_system_time.hour << ":" << SETW(2) << (u16)loc_system_time.minute << ":" << SETW(2) << (u16)loc_system_time.secund; break; // Clock hh:mm:ss
                case 'H':   Format_Filled << SETW(2) << (u16)loc_system_time.hour; break;                                                                                                   // Clock secunde
                case 'M':   Format_Filled << SETW(2) << (u16)loc_system_time.minute; break;                                                                                                 // Clock minute
                case 'S':   Format_Filled << SETW(2) << (u16)loc_system_time.secund; break;                                                                                                 // Clock second
                case 'J':   Format_Filled << SETW(3) << (u16)loc_system_time.millisecends; break;                                                                                           // Clock millisec.

                // ------------------------------------  Date  -------------------------------------------------------------------------------
                case 'N':   Format_Filled << SETW(4) << (u16)loc_system_time.year << "/" << SETW(2) << (u16)loc_system_time.month << "/" << SETW(2) << (u16)loc_system_time.day; break;     // Data yyyy/mm/dd
                case 'Y':   Format_Filled << SETW(4) << (u16)loc_system_time.year; break;                                                                                                   // Year
                case 'O':   Format_Filled << SETW(2) << (u16)loc_system_time.month; break;                                                                                                  // Month
                case 'D':   Format_Filled << SETW(2) << (u16)loc_system_time.day; break;                                                                                                    // Day

                // ------------------------------------  Default  -------------------------------------------------------------------------------
                default: break;
                }

                x++;
            }

            else
                Format_Filled << format_current[x];
        }

#ifdef TIME_FORMATTER_PERFORMANCE
        formatting_stopwatch.stop();
        cumulative_formatting_duration += formating_duration;
        formatting_counter++;
#endif

        {
#ifdef TIME_WRITING_TO_FILE_PERFORMANCE
            f32 write_to_file_duration = 0;
            util::stopwatch write_to_file_stopwatch = util::stopwatch(&write_to_file_duration, util::duration_precision::microseconds);
#endif
            std::lock_guard<std::mutex> file_lock(general_mutex);
            main_file << Format_Filled.str();

#ifdef TIME_WRITING_TO_FILE_PERFORMANCE
            write_to_file_stopwatch.stop();
            cumulative_writing_to_file_duration += write_to_file_duration;
            writing_to_file_counter++;
#endif
        }

        if (write_logs_to_console) {

#ifdef TIME_COUT_PERFORMANCE
            f32 cout_duration = 0;
            util::stopwatch cout_stopwatch = util::stopwatch(&cout_duration, util::duration_precision::microseconds);
#endif
            std::cout << Format_Filled.str();

#ifdef TIME_COUT_PERFORMANCE
            cout_stopwatch.stop();
            cumulative_cout_duration += cout_duration;
            cout_counter++;
#endif
        }

    }
}

// Performance in in Debug
// [LOGGER] Formatting performance:         counter[100020  ] time[4.09833 micro-s]
// [LOGGER] Cout performance:               counter[100020  ] time[2.42007 micro-s]
// [LOGGER] Queue performance:              counter[100020  ] time[1.13808 micro-s]
// [LOGGER] writing to file performance:    counter[100020  ] time[0.42962 micro-s]

// Performance in in Release
// [LOGGER] Formatting performance:         counter[100020  ] time[3.22272 micro-s]
// [LOGGER] Cout performance:               counter[100020  ] time[2.31022 micro-s]
// [LOGGER] Queue performance:              counter[100020  ] time[0.881386 micro-s]
// [LOGGER] writing to file performance:    counter[100020  ] time[0.304581 micro-s]
