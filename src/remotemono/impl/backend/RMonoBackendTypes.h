#pragma once

#include "../../config.h"


namespace remotemono
{
namespace backend
{


enum RMonoProcessorArch
{
	ProcessorArchX86,
	ProcessorArchX86_64
};

enum RMonoCallingConvention
{
	CallConvCdecl,
	CallConvFastcall,
	CallConvStdcall
};


}
}
