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


// ---------------------------------------------------------------------------------------------------------------------------------------
// getters && setters
// ---------------------------------------------------------------------------------------------------------------------------------------

// ------------------------------------------- getters -------------------------------------------
#define DEFAULT_GETTER(type, name)							type get_##name() { return m_##name;}
#define DEFAULT_GETTER_REF(type, name)						type& get_##name##_ref() { return m_##name;}
#define DEFAULT_GETTER_C(type, name)						type get_##name() const { return m_##name;}
#define DEFAULT_GETTER_POINTER(type, name)					type* get_##name##_pointer() { return &m_##name;}

#define DEFAULT_GETTERS(type, name)							DEFAULT_GETTER(type, name)												\
															DEFAULT_GETTER_REF(type, name)											\
															DEFAULT_GETTER_POINTER(type, name)

#define DEFAULT_GETTERS_C(type, name)						DEFAULT_GETTER_C(type, name)											\
															DEFAULT_GETTER_POINTER(type, name)

#define GETTER(type, func_name, var_name)					type get_##func_name() { return var_name;}
#define GETTER_C(type, func_name, var_name)					const type get_##func_name() const { return var_name;}


// ------------------------------------------- setters -------------------------------------------
#define DEFAULT_SETTER(type, name)							void set_##name(type name) { m_##name = name;}
#define SETTER(type, func_name, var_name)					void set_##func_name(type value) { var_name = value;}


// ------------------------------------------- both togetter -------------------------------------------
#define DEFAULT_GETTER_SETTER(type, name)					DEFAULT_GETTER(type, name)												\
															DEFAULT_SETTER(type, name)

#define DEFAULT_GETTER_SETTER_C(type, name)					DEFAULT_GETTER_C(type, name)											\
															DEFAULT_SETTER(type, name)

#define DEFAULT_GETTER_SETTER_ALL(type, name)				DEFAULT_SETTER(type, name)												\
															DEFAULT_GETTER(type, name)												\
															DEFAULT_GETTER_POINTER(type, name)

// ------------------------------------------- function for header -------------------------------------------
#define GETTER_FUNC(type, name)								type get_##name();
#define GETTER_REF_FUNC(type, name)							type& get_##name##_ref();
#define GETTER_C_FUNC(type, name)							type get_##name() const;
#define GETTER_POINTER_FUNC(type, name)						type* get_##name##_pointer();

#define SETTER_FUNC(type, name)								void set_##name(type name);

#define GETTER_SETTER_FUNC(type, name)						GETTER_FUNC(type, name)													\
															SETTER_FUNC(type, name)

// ------------------------------------------- function implementation -------------------------------------------
#define GETTER_FUNC_IMPL(type, name)						type name{};															\
															type get_##name() { return name; }

#define GETTER_FUNC_IMPL2(type, name, value)				type name = value;														\
															type get_##name() { return name; }

#define SETTER_FUNC_IMPL(type, name)						void set_##name(const type new_name) { name = new_name; }

// --------- refrence ---------
#define GETTER_REF_FUNC_IMPL(type, name)					type name{};															\
															type& get_##name##_ref() { return name; }

#define GETTER_REF_FUNC_IMPL2(type, name, value)			type name = value;														\
															type& get_##name##_ref() { return name; }
// --------- const ---------
#define GETTER_C_FUNC_IMPL(type, name)						type name{};															\
															type get_##name() const;

// --------- pointer ---------
#define GETTER_POINTER_FUNC_IMPL(type, name)				type name{};															\
															type* get_##name##_pointer() { return &name; }

#define GETTER_POINTER_FUNC_IMPL2(type, name, value)		type name = value;														\
															type* get_##name##_pointer() { return &name; }



namespace util {

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
    {                                                                                                                                                                                                            \
        f32 duration = -1.f;                                                                                                                                                                                     \
        {   util::stopwatch loc_stpowatch(&duration, profile_duration_precision);                                                                                                                                \
            for (size_t x = 0; x < num_of_iterations; x++) { func }                                                                                                                                              \
        } LOG(Trace, message << " [sample count: " << num_of_iterations << " | average duration: " << (duration / (f64)num_of_iterations) << duration_precision_to_string(profile_duration_precision) << "]");   \
    }


}
