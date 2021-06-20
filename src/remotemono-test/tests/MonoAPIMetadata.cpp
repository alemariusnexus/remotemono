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
#include <remotemono/impl/mono/metadata/blob.h>
#include "../System.h"

namespace fs = std::filesystem;



TEST(MonoAPIMetadataTest, MetadataTables)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto methodTbl = mono.imageGetTableInfo(img, MONO_TABLE_METHOD);
	auto paramTbl = mono.imageGetTableInfo(img, MONO_TABLE_PARAM);

	ASSERT_TRUE(methodTbl);
	ASSERT_TRUE(paramTbl);

	rmono_int methodRows = mono.tableInfoGetRows(methodTbl);
	rmono_int paramRows = mono.tableInfoGetRows(paramTbl);
	EXPECT_GT(methodRows, 0);
	EXPECT_GT(paramRows, 0);

	bool methodFound = false;

	for (rmono_int i = 0 ; i < methodRows ; i++) {
		uint32_t nameGuid = mono.metadataDecodeRowCol(methodTbl, i, MONO_METHOD_NAME);
		std::string name = mono.metadataStringHeap(img, nameGuid);

		if (name == std::string("MethodNameThatShouldBeAsUniqueAsPossible1337420")) {
			uint32_t paramIdxBegin = mono.metadataDecodeRowCol(methodTbl, i, MONO_METHOD_PARAMLIST);
			uint32_t paramIdxEnd;

			if (i < methodRows) {
				paramIdxEnd = mono.metadataDecodeRowCol(methodTbl, i+1, MONO_METHOD_PARAMLIST);
			} else {
				paramIdxEnd = paramRows;
			}

			ASSERT_GT(paramIdxBegin, 0u);
			ASSERT_GT(paramIdxEnd, paramIdxBegin);
			ASSERT_EQ(paramIdxEnd-paramIdxBegin, 2);

			// NOTE: Metadata table indices BEGIN AT 1 (cf. ECMA 335 specification Partition II section 22), but
			// mono_metadata_decode_row*() takes them BEGINNING AT 0. Don't ask me why.
			paramIdxBegin--;
			paramIdxEnd--;

			uint32_t pname1Guid = mono.metadataDecodeRowCol(paramTbl, paramIdxBegin, MONO_PARAM_NAME);
			uint32_t pflags1 = mono.metadataDecodeRowCol(paramTbl, paramIdxBegin, MONO_PARAM_FLAGS);
			std::string pname1 = mono.metadataStringHeap(img, pname1Guid);

			uint32_t pname2Guid = mono.metadataDecodeRowCol(paramTbl, paramIdxBegin+1, MONO_PARAM_NAME);
			uint32_t pflags2 = mono.metadataDecodeRowCol(paramTbl, paramIdxBegin+1, MONO_PARAM_FLAGS);
			std::string pname2 = mono.metadataStringHeap(img, pname2Guid);

			EXPECT_EQ(pname1, std::string("fubar"));
			EXPECT_EQ(pflags1, 0);

			EXPECT_EQ(pname2, std::string("blazeIt"));
			EXPECT_EQ(pflags2, 0x1010); // FlagOptional | FlagHasDefault

			methodFound = true;

			break;
		}
	}

	ASSERT_TRUE(methodFound);
}


TEST(MonoAPIMetadataTest, MetadataBla)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);
}
