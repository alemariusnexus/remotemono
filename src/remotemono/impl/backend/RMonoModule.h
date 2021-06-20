#pragma once

#include "../../config.h"

#include <string>
#include "../RMonoTypes.h"
#include "../exception/RMonoException_Def.h"


namespace remotemono
{
namespace backend
{


class RMonoModule
{
public:
	struct Export
	{
		rmono_funcp procPtr;
	};

public:
	virtual bool getExport(Export& exp, const std::string& name) = 0;

	Export getExport(const std::string& name)
	{
		Export exp;
		if (!getExport(exp, name)) {
			throw RMonoException(std::string("Export not found: ").append(name));
		}
		return exp;
	}

	virtual std::string getName() const = 0;
};


}
}
