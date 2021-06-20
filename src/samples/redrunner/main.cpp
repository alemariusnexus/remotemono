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

#include "pch.h"
#include "config.h"

#include <cstdio>
#include "CLI11.hpp"

using namespace remotemono;


// This is a simple example on how to use RemoteMono. It uses the Unity game RedRunner as a remote. It's best to have
// a copy of RedRunner's source code open when following along with this example. You can get it here:
//
//		https://github.com/BayatGames/RedRunner
//
// To run the example, you have to start RedRunner and enter the game world, then run this example program from the
// console afterwards.


// Keep/remove this to switch between the two sample implementations in main_direct.cpp and main_helpers.cpp. Both of
// them do the exact same thing, but one of them exclusively uses methods provided by RMonoAPI (the direct equivalent
// of the Mono Embedded API), and the other uses the RemoteMono helper classes for simpler and more readable code.
// See main_helpers.cpp and main_direct.cpp for details.
#define REMOTEMONO_SAMPLE_USE_HELPERS



#ifdef REMOTEMONO_SAMPLE_USE_HELPERS
#include "main_helpers.cpp"
#else
#include "main_direct.cpp"
#endif




BOOL WINAPI WinConsoleCtrlHandler(DWORD signal)
{
	// Shutdown handler for console, making sure that RemoteMono will detach (at least on CTRL-C). Otherwise, the remote
	// process will likely crash when it exits.
	if (signal == CTRL_C_EVENT) {
		ShutdownRequested = true;
		return TRUE;
	}
	return FALSE;
}


int main(int argc, char** argv)
{

	// ********** COMMAND-LINE PARSING **********

	CLI::App app("redrunner-sample");

	std::string logLevelStr;
	app.add_option("-l,--log-level", logLevelStr, "The logging level. Valid values are: verbose, debug, info, warning, error, none.");

	CLI11_PARSE(app, argc, argv);

	RMonoLogger::LogLevel logLevel = RMonoLogger::LOG_LEVEL_INFO;

	if (logLevelStr == "none") {
		logLevel = RMonoLogger::LOG_LEVEL_NONE;
	} else if (logLevelStr == "verbose") {
		logLevel = RMonoLogger::LOG_LEVEL_VERBOSE;
	} else if (logLevelStr == "debug") {
		logLevel = RMonoLogger::LOG_LEVEL_DEBUG;
	} else if (logLevelStr == "info") {
		logLevel = RMonoLogger::LOG_LEVEL_INFO;
	} else if (logLevelStr == "warning") {
		logLevel = RMonoLogger::LOG_LEVEL_WARNING;
	} else if (logLevelStr == "error") {
		logLevel = RMonoLogger::LOG_LEVEL_ERROR;
	}



	// ********** SETUP **********

	// Setup logging to stdout
	RMonoStdoutLogFunction::getInstance().registerLogFunction();
	RMonoLogger::getInstance().setLogLevel(logLevel);

	std::wstring exeName(L"RedRunner.exe");

	RMonoLogInfo("Attaching BlackBone ...");

	blackbone::Process proc;

	// Find RedRunner process by executable name
	auto pids = proc.EnumByName(exeName);
	if (pids.empty()) {
		RMonoLogError("Target process not found.");
		return 1;
	} else if (pids.size() > 1) {
		RMonoLogError("Multiple target process candidates found.");
		return 1;
	}

	// Attach BlackBone
	NTSTATUS status = proc.Attach(pids[0]);
	if (!NT_SUCCESS(status)) {
		RMonoLogError("Error attaching to target process.");
		return 1;
	}


	backend::blackbone::RMonoBlackBoneProcess bbProc(&proc);
	RMonoAPI mono(bbProc);

	// Attach RemoteMono
	RMonoLogInfo("Attaching RemoteMono ...");
	mono.attach();

	TestContext ctx;

	ctx.mono = &mono;

	// Setup CTRL-C handler to gracefully shutdown the program (console-only)
	SetConsoleCtrlHandler(WinConsoleCtrlHandler, TRUE);



	// ********** USING THE REMOTEMONO API **********

	// Gather frequently used classes and objects
	RMonoLogInfo("Gathering classes and objects ...");
	SetupTestContext(ctx);

	// Increase various movement speed-related values
	RMonoLogInfo("Increasing movement speed ...");
	IncreaseMovementSpeed(ctx);

	// Enable double-jump
	RMonoLogInfo("Enabling double jump (does not work over water) ...");
	SetupDoubleJump(ctx);

	// Add a UI text element to the bottom-left corner
	RMonoLogInfo("Setting up bottom-left text ...");
	auto testTextObj = AddCanvasText (
			ctx,
			"Hello World from RemoteMono!",
			28,
			260.0f, 15.0f,
			500.0f, 50.0f,
			0.0f, 0.0f
			);

	// Play a sound
	RMonoLogInfo("Playing a lovely little sound ...");
	PlaySound(ctx, "PlayChestSound");

	Sleep(3000);

	// Keep querying main character position and display it in the bottom-left text object
	RMonoLogInfo("Will now keep updating bottom-left text with character position.");
	while (!ShutdownRequested) {
		float x, y;
		GetMainCharacterPosition(ctx, x, y);

		char buf[128];
		sprintf(buf, "Position: %.1f, %.1f", x, y);

		SetCanvasText(ctx, testTextObj, std::string(buf));

		Sleep(50);
	}



	// ********** CLEANUP **********

	RMonoLogInfo("Detaching RemoteMono ...");

	// Detach RemoteMono (optional, destructor would do it automatically)
	mono.detach();

	RMonoLogInfo("*** ALL DONE! ***");

	return 0;
}
