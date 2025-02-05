#ifndef GAME_H
#define GAME_H

#include <vector>
#include "Config.h"
#include "json.hpp"
#include "../entity/Core.h"
#include "../entity/Unit.h"
#include "../action/Action.h"
#include "../net/Bridge.h"

using json = nlohmann::json;

class Game {
public:
    Game(const GameConfig& config);
    // Add a new connection (team or spectator)
    void addBridge(Bridge* bridge);
    // Run the main game loop (blocking call)
    void run();

private:
    void update();       // Update game state (e.g. movement, collisions)
    void sendState();    // Send the current game state over each bridge

    GameConfig config_;
    unsigned int nextEntityId_;
    std::vector<Core> cores_;
    std::vector<Unit> units_;
    std::vector<Bridge*> bridges_;
};

#endif // GAME_H
