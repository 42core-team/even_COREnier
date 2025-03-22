#ifndef ATTACK_ACTION_H
#define ATTACK_ACTION_H

#include <vector>

#include "Action.h"
#include "Common.h"
#include "Money.h"

class Unit;

#include "json.hpp"
using json = nlohmann::ordered_json;

class AttackAction : public Action
{
	public:
		AttackAction(json msg);

		unsigned int getUnitId() const { return unit_id_; }
		Position getDirection() const { return target_pos_; }

		bool execute(Game *game, Core * core);
		void decodeJSON(json msg);
		json encodeJSON();
	
	private:
		unsigned int unit_id_;
		Position target_pos_;
		unsigned int damage_dealt_;
};

#endif // ATTACK_ACTION_H
