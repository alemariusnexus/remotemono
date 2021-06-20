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

#include <cstdio>
#include <windows.h>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>


int main(int argc, char** argv)
{
	MonoDomain* dom = mono_jit_init("remotemono-test-target-dummy-mono.dll");

	MonoAssembly* ass = mono_domain_assembly_open(dom, "remotemono-test-target-dummy-mono.dll");
	if (!ass) {
		fprintf(stderr, "ERROR: Unable to open dummy assembly!\n");
		return 1;
	}

	MonoImage* img = mono_assembly_get_image(ass);
	if (!img) {
		fprintf(stderr, "ERROR: Unable to fetch dummy assembly image!\n");
		return 1;
	}

	MonoClass* cls = mono_class_from_name(img, "", "RemoteMonoTestTargetDummy");


	MonoMethod* dummyMain = mono_class_get_method_from_name(cls, "DummyMain", 0);

	mono_runtime_invoke(dummyMain, nullptr, nullptr, nullptr);

	while (true) {
		Sleep(1000);
	}

	return 0;
}

