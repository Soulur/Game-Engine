#pragma once

#include "src/Core/Base.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#define _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
#define _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS

#pragma warning(pop)



	namespace Mc
{

	class Log
	{
	public:
		static void Init();

		static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};
}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}


// Core log macros
#define LOG_CORE_TRACE(...)		::Mc::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_CORE_INFO(...)		::Mc::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_CORE_WARN(...)		::Mc::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_CORE_ERROR(...)		::Mc::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LOG_CORE_CRITICAL(...)	::Mc::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define LOG_TRACE(...)			::Mc::Log::GetClientLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)			::Mc::Log::GetClientLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)			::Mc::Log::GetClientLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)			::Mc::Log::GetClientLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...)		::Mc::Log::GetClientLogger()->critical(__VA_ARGS__)