#ifndef RESOURCE_H
#define RESOURCE_H

#include "Object.h"
#include "Common.h"
#include "Unit.h"
#include "Config.h"

#include <cmath>

class Resource : public Object
{
	public:
		Resource(unsigned int id, Position pos);
		Resource(unsigned int id, Position pos, unsigned int balance);
		Resource(const Resource &other)
			: Object(other), balance_(other.balance_) {}

		void tick(unsigned long long tickCount);
		void getMined(Unit * miner);

		unsigned int getBalance() const { return balance_; }

	private:
		unsigned int balance_;
};

#endif // RESOURCE_H
