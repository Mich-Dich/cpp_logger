#pragma once

#include <inttypes.h>
#include <chrono>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef	float f32;
typedef double f64;
typedef long double f128;

typedef unsigned long long handle;

namespace util {

    // ---------------------------------------------------------------------------------------------------------------------------------------
    // getters
    // ---------------------------------------------------------------------------------------------------------------------------------------
    #define DEFAULT_GETTER(type, name)							type get_##name() { return m_##name;}
    #define DEFAULT_GETTER_REF(type, name)						type& get_##name##_ref() { return m_##name;}
    #define DEFAULT_GETTER_C(type, name)						type get_##name() const { return m_##name;}
    #define DEFAULT_GETTER_POINTER(type, name)					type* get_##name##_pointer() { return &m_##name;}

	struct system_time {

		u16 year;
		u8 month;
		u8 day;
		u8 day_of_week;
		u8 hour;
		u8 minute;
		u8 secund;
		u16 millisecends;
	};

	enum class duration_precision : u8 {
		microseconds,
		milliseconds,
		seconds,
	};
    inline const char* duration_precision_to_string(const duration_precision dp) {
        switch (dp) {
            case duration_precision::microseconds: return " micro-s";
            case duration_precision::milliseconds: return " ms";
            case duration_precision::seconds: return " s";
            default: return " unknown";
        }
    }

    system_time get_system_time();


    // @brief This is a lightweit stopwatch that is automaticlly started when creating an instance.
    //        It can either store the elapsed time in a provided float pointer when the stopwatch is stopped/destroyed, 
    //        or it can allow retrieval of the elapsed time by manually calling [stop()] method.
    //        The time is measured in milliseconds.
    class stopwatch {
    public:

        stopwatch(f32* result_pointer, duration_precision presition = duration_precision::milliseconds)
            : m_result_pointer(result_pointer), m_presition(presition) { _start(); }

        ~stopwatch() { stop(); }

        DEFAULT_GETTER_C(f32, result);

        // @brief Stops the stopwatch and calculates the elapsed time.
        //        If a result pointer was provided, it will be updated with the elapsed time.
        // @return The elapsed time in milliseconds since the stopwatch was started.
        f32 stop();

        // @breif Restarts the stopped stopwatch
        void restart();

    private:

        void _start();

        duration_precision                          m_presition;
        std::chrono::system_clock::time_point       m_start_point{};
        f32                                         m_result = 0.f;
        f32*                                        m_result_pointer = &m_result;
    };

#define ISOLATED_PROFILER_LOOP(num_of_iterations, message, profile_duration_precision, func)                                                                                                                     \
    {   f32 duration = -1.f;                                                                                                                                                                                     \
        {   util::stopwatch loc_stpowatch(&duration, profile_duration_precision);                                                                                                                                \
            for (size_t x = 0; x < num_of_iterations; x++) { func }                                                                                                                                              \
        } LOG(Trace, message << " [sample count: " << num_of_iterations << " | average duration: " << (duration / (f64)num_of_iterations) << duration_precision_to_string(profile_duration_precision) << "]"); }

}
