/*
	Copyright 2020-2021 David "Alemarius Nexus" Lerch

	This file is part of RemoteMono.

	RemoteMono is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	RemoteMono is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with RemoteMono.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "config.h"

#include <cstdarg>
#include <ctime>
#include <cstdio>
#include <string>
#include <functional>
#include <list>
#include <windows.h>




namespace remotemono
{


/**
 * The singleton class through which RemoteMono logs its various internal operations. You can receive the messages logged
 * by RemoteMono by registering a log function with `registerLogFunction()`.
 *
 * You can also use RMonoStdoutLogFunction::registerLogFunction() for quick and simple logging to stdout.
 *
 * If you want to actually log a message, consider using one of the global RMonoLog*() functions instead of directly using
 * this class.
 */
class RMonoLogger
{
public:
	enum LogLevel
	{
		LOG_LEVEL_NONE = 0,
		LOG_LEVEL_ERROR = 10,
		LOG_LEVEL_WARNING = 20,
		LOG_LEVEL_INFO = 30,
		LOG_LEVEL_DEBUG = 40,
		LOG_LEVEL_VERBOSE = 50
	};

	struct LogMessage
	{
		std::string_view msg;
		LogLevel level;
	};

	typedef std::function<void (const LogMessage&)> LogFunction;
	typedef int LogFunctionID;

private:
	struct LogFuncEntry
	{
		LogFuncEntry(LogFunction f, LogFunctionID id) : f(f), id(id) {}

		LogFunction f;
		LogFunctionID id;
	};

public:
	static RMonoLogger& getInstance()
	{
		static RMonoLogger inst;
		return inst;
	}

	void setLogLevel(LogLevel level) { this->level = level; }
	LogLevel getLogLevel() const { return level; }

	const char* getLogLevelName(LogLevel level)
	{
		switch (level) {
		case LOG_LEVEL_NONE:	return "none";
		case LOG_LEVEL_ERROR:	return "error";
		case LOG_LEVEL_WARNING:	return "warning";
		case LOG_LEVEL_INFO:	return "info";
		case LOG_LEVEL_DEBUG:	return "debug";
		case LOG_LEVEL_VERBOSE:	return "verbose";
		}
		return "[INVALID]";
	}

	bool isLogLevelActive(LogLevel level) { return this->level >= level; }

	LogFunctionID registerLogFunction(LogFunction f)
	{
		LogFunctionID id = nextLogFuncID++;
		logFuncs.emplace_back(f, id);
		return id;
	}

	bool unregisterLogFunction(LogFunctionID id)
	{
		for (auto it = logFuncs.begin() ; it != logFuncs.end() ; it++) {
			if (it->id == id) {
				logFuncs.erase(it);
				return true;
			}
		}
		return false;
	}

	template <typename... Args>
	void logMessageUnchecked(LogLevel level, const char* fmt, Args... args)
	{
		logMessagevUnchecked(level, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void logMessage(LogLevel level, const char* fmt, Args... args)
	{
		if (isLogLevelActive(level)  &&  !logFuncs.empty()) {
			logMessageUnchecked(level, fmt, args...);
		}
	}

private:
	RMonoLogger() : level(LOG_LEVEL_INFO), nextLogFuncID(1)
	{
		mtx = CreateMutex(NULL, false, NULL);
	}

	void lockMutex() { WaitForSingleObject(mtx, INFINITE); }
	void unlockMutex() { ReleaseMutex(mtx); }

	void logMessagevlUnchecked(LogLevel level, const char* fmt, va_list args)
	{
		// Don't check for log level. This is done by logMessage()

		lockMutex();

		va_list argsCpy;

		va_copy(argsCpy, args);
		int len = vsnprintf(nullptr, 0, fmt, argsCpy);
		va_end(argsCpy);

		char* msgStr = new char[len+1];

		va_copy(argsCpy, args);
		vsnprintf(msgStr, len+1, fmt, argsCpy);
		va_end(argsCpy);

		LogMessage msg;
		msg.msg = std::string_view(msgStr, len);
		msg.level = level;

		for (LogFuncEntry& e : logFuncs) {
			e.f(msg);
		}

		delete[] msgStr;

		unlockMutex();
	}

	void logMessagevUnchecked(LogLevel level, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		logMessagevlUnchecked(level, fmt, args);
		va_end(args);
	}

private:
	LogLevel level;
	std::list<LogFuncEntry> logFuncs;
	LogFunctionID nextLogFuncID;

	HANDLE mtx;
};






/**
 * A simple logging function that logs messages to stdout using printf().
 */
class RMonoStdoutLogFunction
{
public:
	static RMonoStdoutLogFunction& getInstance()
	{
		static RMonoStdoutLogFunction inst;
		return inst;
	}

public:
	void registerLogFunction()
	{
		logFuncID = RMonoLogger::getInstance().registerLogFunction(*this);
	}

	bool unregisterLogFunction()
	{
		bool res = RMonoLogger::getInstance().unregisterLogFunction(logFuncID);
		logFuncID = 0;
		return res;
	}

	void operator()(const RMonoLogger::LogMessage& msg)
	{
		time_t t = time(NULL);

		const char* typeCode = "[???]";

		switch (msg.level) {
		case RMonoLogger::LOG_LEVEL_ERROR:
			typeCode = "[ERR]";
			break;
		case RMonoLogger::LOG_LEVEL_WARNING:
			typeCode = "[WRN]";
			break;
		case RMonoLogger::LOG_LEVEL_INFO:
			typeCode = "[INF]";
			break;
		case RMonoLogger::LOG_LEVEL_DEBUG:
			typeCode = "[DBG]";
			break;
		case RMonoLogger::LOG_LEVEL_VERBOSE:
			typeCode = "[VRB]";
			break;
		}

		char timeStr[128];

		struct tm localTime;
		localtime_s(&localTime, &t);
		strftime(timeStr, sizeof(timeStr), timeFormat.data(), &localTime);

		printf("%s %s - %s\n", typeCode, timeStr, msg.msg.data());
		fflush(stdout);
	}

	void setTimeFormat(const std::string& format) { timeFormat = format; }

private:
	RMonoStdoutLogFunction() {}

private:
	std::string timeFormat = "%Y-%m-%d %H:%M:%S";
	RMonoLogger::LogFunctionID logFuncID = 0;
};






template <class... T> void RMonoLogError(const char* fmt, T... args) { RMonoLogger::getInstance().logMessage(RMonoLogger::LOG_LEVEL_ERROR, fmt, std::forward<T>(args)...); }
template <class... T> void RMonoLogWarning(const char* fmt, T... args) { RMonoLogger::getInstance().logMessage(RMonoLogger::LOG_LEVEL_WARNING, fmt, std::forward<T>(args)...); }
template <class... T> void RMonoLogInfo(const char* fmt, T... args) { RMonoLogger::getInstance().logMessage(RMonoLogger::LOG_LEVEL_INFO, fmt, std::forward<T>(args)...); }
template <class... T> void RMonoLogDebug(const char* fmt, T... args) { RMonoLogger::getInstance().logMessage(RMonoLogger::LOG_LEVEL_DEBUG, fmt, std::forward<T>(args)...); }
template <class... T> void RMonoLogVerbose(const char* fmt, T... args) { RMonoLogger::getInstance().logMessage(RMonoLogger::LOG_LEVEL_VERBOSE, fmt, std::forward<T>(args)...); }


}

