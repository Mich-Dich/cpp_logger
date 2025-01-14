
#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <map>
#include <string>
#include <cstring>
#include <fstream>
#include <csignal>

#if defined __WIN32__
    #include <Windows.h>
#elif defined __unix__
    #include <time.h>
    #include <sys/time.h>
    #include <ctime>
#endif

#include "util.h"
#include "logger.h"

namespace logger {

#define SETW(width)                                 std::setw(width) << std::setfill('0')

#define PROJECT_FOLDER                              "PFF"

#define SHORTEN_FILE_PATH(text)                     (strstr(text, PROJECT_FOLDER) ? strstr(text, PROJECT_FOLDER) + strlen(PROJECT_FOLDER) + 1 : text)
#define SHORT_FILE(text)                            (strrchr(text, "\\") ? strrchr(text, "\\") + 1 : text)
#define SHORTEN_FUNC_NAME(text)                     (strstr(text, "::") ? strstr(text, "::") + 2 : text)


    static bool                                     is_init = false;
    static std::string                              format_current = "";
    static std::string                              format_prev = "";
    static severity                                 sev_level_to_buffer = severity::Trace;
    static u32                                      max_size_of_buffer = 0;
    static std::filesystem::path                    main_log_dir = "";
    static std::filesystem::path                    main_log_file_path = "";
    static std::ofstream                            main_file;
    static const char*                              severity_names[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
    static const char*                              console_reset = "\x1b[0m";
    static const char*                              console_color_table[] = {
        "\x1b[38;5;246m",     // Trace: Gray
        "\x1b[94m",           // Debug: Blue
        "\x1b[92m",           // Info: Green
        "\x1b[33m",           // Warn: Yellow
        "\x1b[31m",           // Error: Red
        "\x1b[41m\x1b[30m",   // Fatal: Red Background
    };

    static std::thread                              worker_thread;
    static std::condition_variable                  cv{};
    static std::mutex                               queue_mutex{};
    static std::mutex                               file_mutex{};
    static std::atomic<bool>                        ready = false;
    static std::atomic<bool>                        stop = false;
    static std::queue<message_format>               log_queue{};
    static std::map<std::thread::id, std::string>   thread_lable_map = {};

    void process_queue();
    void process_log_message(const message_format message);

    inline static const char* get_filename(const char* filepath) {

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

    bool init(const std::string& format, const std::filesystem::path log_dir, const std::string& main_log_file_name, const bool use_append_mode) {

        if (is_init)
            DEBUG_BREAK("Tryed to init lgging system multiple times")

        format_current = format;
        format_prev = format;

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

        CLOSE_MAIN_FILE()
        is_init = true;
        return true;
    }

    void shutdown() {

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        cv.notify_all();
        
        if (worker_thread.joinable())
            worker_thread.join();

        if (main_file.is_open())
            CLOSE_MAIN_FILE()

        is_init = false;
    }

    // ====================================================================================================================================
    // settings
    // ====================================================================================================================================

    void set_format(const std::string& new_format) {

        {
            std::lock_guard<std::mutex> lock(file_mutex);
            OPEN_MAIN_FILE(true)
            format_prev = format_current;
            format_current = new_format;
            main_file << "[LOGGER] Changing log-format. From [" << format_prev << "] to [" << format_current << "]\n";
            CLOSE_MAIN_FILE()
        }
    }

    void use_previous_format() {

        const std::string buffer = format_current;
        format_current = format_prev;
        format_prev = buffer;

        {
            std::lock_guard<std::mutex> lock(file_mutex);
            OPEN_MAIN_FILE(true);
            main_file << "[LOGGER] Changing to previous log-format. From [" << format_prev << "] to [" << format_current << "]\n";
            CLOSE_MAIN_FILE()
        }
    }

    const std::string get_format() { return format_current; }

    void set_buffer_settings(const severity sev_level, const u32 new_max_size_of_buffer) {
        
        sev_level_to_buffer = sev_level;
        max_size_of_buffer = new_max_size_of_buffer;
    }

    void register_label_for_thread(const std::string& thread_lable, std::thread::id thread_id) {

        std::lock_guard<std::mutex> lock(file_mutex);
        OPEN_MAIN_FILE(true);
        if (thread_lable_map.find(thread_id) != thread_lable_map.end()) {

            main_file << "[LOGGER] Thread with ID: [" << thread_id << "] already has lable [" << thread_lable_map[thread_id] << "] registered. Overriding with the lable: [" << thread_lable << "]\n";
            thread_lable_map[thread_id] = thread_lable;
        }
        else {

            main_file << "[LOGGER] Registering Thread-ID: [" << thread_id << "] with the lable: [" << thread_lable << "]\n";
            thread_lable_map.emplace(thread_id, thread_lable);
        }
        CLOSE_MAIN_FILE()
    }

    void unregister_label_for_thread(std::thread::id thread_id) {

        if (thread_lable_map.find(thread_id) == thread_lable_map.end()) {
            
            std::lock_guard<std::mutex> lock(file_mutex);
            OPEN_MAIN_FILE(true)
            main_file << "[LOGGER] Tried to unregister lable for Thread-ID: [" << thread_id << "]. IGNORED\n";
            CLOSE_MAIN_FILE()
            return;
        }

        std::lock_guard<std::mutex> lock(file_mutex);
        OPEN_MAIN_FILE(true)
        main_file << "[LOGGER] Unregistering Thread-ID: [" << thread_id << "] with the lable: [" << thread_lable_map[thread_id] << "]\n";
        CLOSE_MAIN_FILE()

        thread_lable_map.erase(thread_id);
    }

    // ====================================================================================================================================
    // log message handeling
    // ====================================================================================================================================

    void shutdown_signal(const int signal) {
        
        std::cerr << "Detected interup signal!!  shuting down logger" << std::endl;
        logger::shutdown();    
    }
    void shutdown_atexit() {
        
        std::cerr << "Detected interup signal!!  shuting down logger" << std::endl;
        logger::shutdown();    
    }

    void process_queue() {

        std::signal(SIGINT, shutdown_signal);
        std::signal(SIGTERM, shutdown_signal);
        std::atexit([]() { shutdown(); } );

        while (!stop || !log_queue.empty()) { // Continue until stop is true and the queue is empty

            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [] { return !log_queue.empty(); });

            std::lock_guard<std::mutex> file_lock(file_mutex);
            OPEN_MAIN_FILE(true);
            while (!log_queue.empty()) {

                // get message from queue
                if (!lock.owns_lock())
                    lock.lock();
                message_format message = std::move(log_queue.front());
                log_queue.pop();
                lock.unlock();

                process_log_message(std::move(message)); // Process the message (format and write to file)
            }
            CLOSE_MAIN_FILE()
        }
    }

    void log_msg(const severity msg_sev , const char* file_name, const char* function_name, const int line, const std::thread::id thread_id, const std::string& message) {
        
        if (message.empty())
            return;                      // dont log empty lines

        if (!is_init)
            DEBUG_BREAK("logger::log_msg() was called bevor initalizing logger")

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            log_queue.emplace(msg_sev, file_name, function_name, line, thread_id, std::move(message));
        }
        cv.notify_all();
    }

    void process_log_message(const message_format message) {

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
                case 'Z':   Format_Filled << std::endl; break;                                                                                                                              // Alignment

                // ------------------------------------  Source  -------------------------------------------------------------------------------
                case 'Q':   if (thread_lable_map.find(message.thread_id) != thread_lable_map.end()) {Format_Filled << thread_lable_map[message.thread_id]; } else { Format_Filled << message.thread_id; } break;      // Thread id or asosiated lable
                case 'F':   Format_Filled << message.function_name; break;                                                                                                                  // Function Name
                case 'P':   Format_Filled << SHORTEN_FUNC_NAME(message.function_name); break;                                                                                               // Function Name
                case 'A':   Format_Filled << message.file_name; break;                                                                                                                      // File Name
                case 'K':   Format_Filled << SHORTEN_FILE_PATH(message.file_name); break;                                                                                                   // Shortend File Name
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

        main_file << Format_Filled.str();
        std::cout << Format_Filled.str();
    }

}
