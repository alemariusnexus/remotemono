#include "TestBackend.h"


std::vector<TestBackend*> TestBackend::supportedBackends;



TestBackend* TestBackend::getDefaultBackend()
{
	TestBackend* bestBe = nullptr;
	for (TestBackend* be : getSupportedBackends()) {
		if (!bestBe  ||  (be->getPriority() < bestBe->getPriority())) {
			bestBe = be;
		}
	}
	return bestBe;
}
