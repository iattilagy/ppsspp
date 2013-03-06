// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.


// TO USE:
// Simply copy pspautotests to the root of the USB memory / SD card of your android device.
// Then go to Settings / Developer Menu / Run CPU tests.
// It currently just runs one test but that can be easily changed.

#include <string>
#include <sstream>
#include <iostream>

#include "base/basictypes.h"
#include "base/logging.h"

#include "Core/Core.h"
#include "Core/System.h"
#include "Core/Config.h"
#include "Core/CoreTiming.h"
#include "Core/MIPS/MIPS.h"
#include "TestRunner.h"


const char *testsToRun[] = {
	"cpu/cpu_alu/cpu_alu",
	"cpu/fpu/fpu",
	"cpu/icache/icache",
	"cpu/lsu/lsu",
	"cpu/vfpu/base/vfpu",
	"cpu/vfpu/colors/vfpu_colors",
	"cpu/vfpu/convert/vfpu_convert",
	"cpu/vfpu/prefixes/vfpu_prefixes",
};

void RunTests()
{
	std::string output;

	CoreParameter coreParam;
	coreParam.cpuCore = g_Config.bJit ? CPU_JIT : CPU_INTERPRETER;
	coreParam.gpuCore = GPU_GLES;
	coreParam.enableSound = g_Config.bEnableSound;
	coreParam.mountIso = "";
	coreParam.startPaused = false;
	coreParam.enableDebugging = false;
	coreParam.printfEmuLog = false;
	coreParam.headLess = false;
	coreParam.renderWidth = 480;
	coreParam.renderHeight = 272;
	coreParam.outputWidth = 480;
	coreParam.outputHeight = 272;
	coreParam.pixelWidth = 480;
	coreParam.pixelHeight = 272;
	coreParam.useMediaEngine = false;
	coreParam.collectEmuLog = &output;

	for (int i = 0; i < ARRAY_SIZE(testsToRun); i++) {
		const char *testName = testsToRun[i];
		coreParam.fileToStart = g_Config.memCardDirectory + "/pspautotests/tests/" + testName + ".prx";
		std::string expectedFile =  g_Config.memCardDirectory + "/pspautotests/tests/" + testName + ".expected";

		ILOG("Preparing to execute %s", testName)
		std::string error_string;
		output = "";
		if (!PSP_Init(coreParam, &error_string)) {
			ELOG("Failed to init unittest %s : %s", testsToRun[i], error_string.c_str());
			return;
		}

		// Run the emu until the test exits
		while (true) {
			int blockTicks = usToCycles(1000000 / 10);
			while (coreState == CORE_RUNNING) {
				u64 nowTicks = CoreTiming::GetTicks();
				mipsr4k.RunLoopUntil(nowTicks + blockTicks);
			}
			// Hopefully coreState is now CORE_NEXTFRAME
			if (coreState == CORE_NEXTFRAME) {
				// set back to running for the next frame
				coreState = CORE_RUNNING;
			} else if (coreState == CORE_POWERDOWN)	{
				ILOG("Finished running test %s", testName);
				break;
			}
		}
	
		std::ifstream expected(expectedFile.c_str(), std::ios_base::in);
		if (!expected) {
			ELOG("Error opening expectedFile %s", expectedFile.c_str());
			return;
		}

		std::istringstream logoutput(output);

		while (true) {
			std::string e, o;
			std::getline(expected, e);
			std::getline(logoutput, o);
			e = e.substr(0, e.size() - 1);  // For some reason we get some extra character
			if (e != o) {
				ELOG("DIFF! %i vs %i, %s vs %s", (int)e.size(), (int)o.size(), e.c_str(), o.c_str());
			}
			if (expected.eof()) {
				break;
			}
			if (logoutput.eof()) {
				break;
			}
		}

		ILOG("Test executed.");
		return;
	}
}