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

#include "../config.h"

#include <string>
#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include "../System.h"

namespace fs = std::filesystem;



TEST(MonoAPIImageTest, ImageName)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	EXPECT_TRUE(img);

	std::string name = mono.imageGetName(img);
	std::string fname = mono.imageGetFilename(img);

	EXPECT_EQ(name, std::string("remotemono-test-target-mono"));
	EXPECT_EQ(fs::path(fname).filename().string(), std::string("remotemono-test-target-mono.dll"));
}
