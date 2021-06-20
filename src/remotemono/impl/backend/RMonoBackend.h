#pragma once

#include "../../config.h"


namespace remotemono
{
namespace backend
{


class RMonoBackend
{
public:
	virtual std::string getID() const = 0;
	virtual std::string getName() const = 0;
};


}
}
