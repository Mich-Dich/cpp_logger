
#include "util.h"

#include <chrono>

#include <sys/time.h>
#include <string>
//#include <ctime>

#include <QApplication>
#include <QFileDialog>
#include <QString>
#include <filesystem>

namespace util {

    system_time get_system_time() {

        system_time loc_system_time{};

#if defined(__WIN32__)

        SYSTEMTIME win_time;
        GetLocalTime(&win_time);
        loc_system_time.year = static_cast<u16>(win_time.wYear);
        loc_system_time.month = static_cast<u8>(win_time.wMonth);
        loc_system_time.day = static_cast<u8>(win_time.wDay);
        loc_system_time.day_of_week = static_cast<u8>(win_time.wDayOfWeek);
        loc_system_time.hour = static_cast<u8>(win_time.wHour);
        loc_system_time.minute = static_cast<u8>(win_time.wMinute);
        loc_system_time.secund = static_cast<u8>(win_time.wSecond);
        loc_system_time.millisecends = static_cast<u16>(win_time.wMilliseconds);

#elif defined(__unix__)

        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm* ptm = localtime(&tv.tv_sec);
        loc_system_time.year = static_cast<u16>(ptm->tm_year + 1900);
        loc_system_time.month = static_cast<u8>(ptm->tm_mon + 1);
        loc_system_time.day = static_cast<u8>(ptm->tm_mday);
        loc_system_time.day_of_week = static_cast<u8>(ptm->tm_wday);
        loc_system_time.hour = static_cast<u8>(ptm->tm_hour);
        loc_system_time.minute = static_cast<u8>(ptm->tm_min);
        loc_system_time.secund = static_cast<u8>(ptm->tm_sec);
        loc_system_time.millisecends = static_cast<u16>(tv.tv_usec / 1000);

#endif
        return loc_system_time;
    }

    std::filesystem::path file_dialog(const std::string_view title, const std::vector<std::pair<std::string, std::string>>& filters) {
        
        int argc = 0;
        char **argv = nullptr;
        QApplication app(argc, argv);                                                                                                   // Create a QApplication instance

        QString filterString;
        for (auto& filter : filters) {                                                                                                  // Prepare the filter string for QFileDialog
            
            // Replace semicolons with spaces
            std::string buffer = filter.second;
            size_t pos = 0;
            while ((pos = buffer.find(';', pos)) != std::string::npos) {                                                                // Replace ';' with ' ' 
                buffer.replace(pos, 1, " ");
                pos += 1;
            }
            
            filterString += QString::fromStdString(filter.first) + " (" + QString::fromStdString(buffer) + ");;";
        }
        filterString.chop(2); // Remove the last ";;"

        QString fileName = QFileDialog::getOpenFileName(nullptr, QString::fromUtf8(title.data()), QString(), filterString);             // Open the file dialog
        return std::filesystem::path(fileName.toStdString());
    }

    f32 stopwatch::stop() {

        std::chrono::system_clock::time_point end_point = std::chrono::system_clock::now();
        switch (m_presition) {
            case duration_precision::microseconds: return *m_result_pointer = std::chrono::duration_cast<std::chrono::nanoseconds>(end_point - m_start_point).count() / 1000.f;
            case duration_precision::seconds:      return *m_result_pointer = std::chrono::duration_cast<std::chrono::milliseconds>(end_point - m_start_point).count() / 1000.f;
            default:
            case duration_precision::milliseconds: return *m_result_pointer = std::chrono::duration_cast<std::chrono::microseconds>(end_point - m_start_point).count() / 1000.f;
        }
    }

    void stopwatch::restart() {

        _start();
    }


    void stopwatch::_start() { m_start_point = std::chrono::system_clock::now(); }

}