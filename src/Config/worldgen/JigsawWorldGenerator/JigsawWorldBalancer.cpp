#include "JigsawWorldGenerator.h"

void JigsawWorldGenerator::balanceResourceAmount(Game* game)
{
	int max_iter = 1000;

	std::vector<size_t> resourceIndices;
	for (size_t i = 0; i < game->getObjects().size(); ++i)
	{
		bool canBeRemoved = true;
		if (game->getObjects()[i]->getType() != ObjectType::Resource)
			canBeRemoved = false;
		for (const auto &safeZone : coreWallRegions_)
		{
			if (game->getObjects()[i]->getPosition().x >= safeZone.x &&
				game->getObjects()[i]->getPosition().x < safeZone.x + safeZone.width &&
				game->getObjects()[i]->getPosition().y >= safeZone.y &&
				game->getObjects()[i]->getPosition().y < safeZone.y + safeZone.height)
			{
				canBeRemoved = false;
				break;
			}
		}

		if (canBeRemoved)
			resourceIndices.push_back(i);
	}
	int currentResources = resourceIndices.size();
	
	if (currentResources > expectedResourceCount_)
	{
		int removeCount = currentResources - expectedResourceCount_;

		std::shuffle(resourceIndices.begin(), resourceIndices.end(), eng_);

		std::vector<size_t> indicesToRemove(resourceIndices.begin(), resourceIndices.begin() + removeCount);
		std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<size_t>());

		for (size_t idx : indicesToRemove)
		{
			game->getObjects().erase(game->getObjects().begin() + idx);
		}
	}
	else if (currentResources < expectedResourceCount_)
	{
		int addCount = expectedResourceCount_ - currentResources;
		int iter = 0;
		std::uniform_int_distribution<int> distX(0, Config::getInstance().width - 1);
		std::uniform_int_distribution<int> distY(0, Config::getInstance().height - 1);

		while (addCount > 0 && ++iter < max_iter)
		{
			int x = distX(eng_);
			int y = distY(eng_);
			Position pos(x, y);

			if (game->getObjectAtPos(pos) == nullptr)
			{
				bool nearCore = false;
				for (const auto &safeZone : coreWallRegions_)
				{
					if (pos.x >= safeZone.x && pos.x < safeZone.x + safeZone.width &&
						pos.y >= safeZone.y && pos.y < safeZone.y + safeZone.height)
					{
						nearCore = true;
						break;
					}
				}
				if (nearCore)
					continue;

				game->getObjects().push_back(std::make_unique<Resource>(game->getNextObjectId(), pos));

				if (!areAllCoresConnected(game))
					game->getObjects().pop_back();

				addCount--;
			}
		}
	}
}

static bool isWithinBounds(int x, int y, unsigned int width, unsigned int height)
{
	return (x >= 0 && x < (int)width && y >= 0 && y < (int)height);
}

// Dijkstra helper node
struct Node {
	int cost;
	Position pos;
	bool operator>(const Node &other) const {
		return cost > other.cost;
	}
};

// map of how many steps it takes to get to each position a given start position
std::vector<std::vector<int>> computeDistanceMap(Game* game, const Position& start)
{
	unsigned int width = Config::getInstance().width;
	unsigned int height = Config::getInstance().height;
	std::vector<std::vector<int>> dist(height, std::vector<int>(width, std::numeric_limits<int>::max()));
	
	double sumWallDamage = 0;
	double sumResourceDamage = 0;
	const auto &units = Config::getInstance().units;
	for (const auto &unit : units)
	{
		sumWallDamage += unit.damageWall;
		sumResourceDamage += unit.damageResource;
	}
	double avgWallDamage = (units.size() > 0) ? (sumWallDamage / units.size()) : 1.0;
	double avgResourceDamage = (units.size() > 0) ? (sumResourceDamage / units.size()) : 1.0;
	
	int wallCost = std::ceil((double)Config::getInstance().wallHp / avgWallDamage);
	int resourceCost = std::ceil((double)Config::getInstance().resourceHp / avgResourceDamage);
	
	std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
	dist[start.y][start.x] = 0;
	pq.push({0, start});
	
	std::vector<Position> directions = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
	
	while (!pq.empty())
	{
		Node node = pq.top();
		pq.pop();
		Position cur = node.pos;
		int curCost = node.cost;
		
		if (curCost > dist[cur.y][cur.x])
			continue;
		
		for (const auto &d : directions)
		{
			int nx = cur.x + d.x;
			int ny = cur.y + d.y;
			if (isWithinBounds(nx, ny, width, height)) 
			{
				Position neighbor(nx, ny);
				int stepCost = 1;
				
				auto obj = game->getObjectAtPos(neighbor);
				if (obj != nullptr)
				{
					if (obj->getType() == ObjectType::Wall)
						stepCost = wallCost;
					else if (obj->getType() == ObjectType::Resource)
						stepCost = resourceCost;
				}
				
				if (dist[ny][nx] > curCost + stepCost)
				{
					dist[ny][nx] = curCost + stepCost;
					pq.push({dist[ny][nx], neighbor});
				}
			}
		}
	}
	return dist;
}

// if you need to change anything about this, may god have mercy on your soul
void JigsawWorldGenerator::balanceResourceDistribution(Game* game)
{
	const auto& cores = game->getCores();
	if (cores.empty())
		return; 

	unsigned int width = Config::getInstance().width;
	unsigned int height = Config::getInstance().height;

	// 1. Compute distance maps for each core.
	std::map<unsigned int, std::vector<std::vector<int>>> coreDistanceMaps; // map < core id, vert vector < horiz vector < distance > > >
	for (const auto &core : cores)
		coreDistanceMaps[core.getId()] = computeDistanceMap(game, core.getPosition());

	// 2. Assign each resource to the closest core by Dijkstra distance.
	std::map<unsigned int, std::vector<size_t>> coreResources; // map < core id, vector < resource id > >
	for (size_t i = 0; i < game->getObjects().size(); ++i)
	{
		auto &obj = game->getObjects()[i];
		if (obj->getType() != ObjectType::Resource)
			continue;
		Position pos = obj->getPosition();
		int bestDijkstra = std::numeric_limits<int>::max();
		unsigned int bestCoreId = 0;
		for (size_t c = 0; c < cores.size(); ++c)
		{
			int dijkstra = coreDistanceMaps[cores[c].getId()][pos.y][pos.x];
			if (dijkstra < bestDijkstra)
			{
				bestDijkstra = dijkstra;
				bestCoreId = cores[c].getId();
			}
		}
		coreResources[bestCoreId].push_back(game->getObjects()[i]->getId());
	}

	// 3. Build frequency maps for each core: distance -> count.
	// Also record for each resource its computed Dijkstra distance from its assigned core.
	std::map<unsigned int, std::map<int, int>> coreFreq; // map < core id, res counts map < res dist, res dist count > >
	std::map<unsigned int, int> resourceDistance; // map < res id, res dist >

	for (size_t c = 0; c < cores.size(); ++c)
	{
		for (unsigned int resId : coreResources[c])
		{
			auto objPtr = game->getObjectsMap()[resId].lock();
			if (!objPtr)
				continue;
			Position pos = objPtr->getPosition();
			int d = coreDistanceMaps[c][pos.y][pos.x];
			coreFreq[cores[c].getId()][d]++;
			resourceDistance[resId] = d;
		}
	}

	// 4. Begin rebalancing until equilibrium: difference at any distance is <= 1.
	const int MAX_ITER = 1000;
	int iter = 0;
	while (iter < MAX_ITER)
	{
		bool balanced = true;
		// every distance level that appears in any core's frequency map
		std::set<int> allDistances;
		for (const auto &freqMap : coreFreq)
		{
			for (const auto &p : freqMap.second)
				allDistances.insert(p.first);
		}

		// Check imbalance at each distance level.
		for (int d : allDistances)
		{
			// determine max & mins for that distance level
			int minCount = std::numeric_limits<int>::max();
			int maxCount = -1;
			int coreMinId = 0, coreMaxId = 0;
			for (size_t c = 0; c < cores.size(); ++c)
			{
				int count = (coreFreq[c].count(d) ? coreFreq[c].at(d) : 0);
				if (count < minCount)
				{
					minCount = count;
					coreMinId = cores[c].getId();
				}
				if (count > maxCount)
				{
					maxCount = count;
					coreMaxId = cores[c].getId();
				}
			}

			// if the difference is 0 or 1, this distance level is balanced
			if (maxCount - minCount >= 1)
				continue;

			balanced = false;
			// Core 'coreMax' has an excess resource at distance d.
			// Let's choose a resource in coreMax with that distance.
			size_t resourceToMove = SIZE_MAX;
			for (auto resIdx : coreResources[coreMaxId])
			{
				if (resourceDistance[resIdx] == d)
				{
					resourceToMove = resIdx;
					break;
				}
			}
			if (resourceToMove == SIZE_MAX)
			{
				// what the heck
				Logger::Log(LogLevel::ERROR, "Resource balancing error: no resource found to move. This should not happen given common laws of logic.");
				continue;
			}

			// Remove this resource from coreMax's record:
			coreFreq[coreMaxId][d]--;
			// Erase resourceToMove from coreResources[coreMax]:
			auto it = std::find(coreResources[coreMaxId].begin(), coreResources[coreMaxId].end(), resourceToMove);
			if (it != coreResources[coreMaxId].end())
				coreResources[coreMaxId].erase(it);

			// TODO: insert at the closest to core position thats not matching another close position instead of just at the same distance
			// TODO: move this entire function away from arbitrary array indexes to maps with the normal object ids as keys
			// Now, we need to find a new position on coreMin's side.
			Position newPos(-1, -1);
			const auto &dijkMap = coreDistanceMaps[coreMinId];
			bool found = false;
			for (int y = 0; y < (int)height && !found; ++y)
			{
				for (int x = 0; x < (int)width && !found; ++x)
				{
					if (dijkMap[y][x] != d)
						continue;
					if (game->getObjectAtPos(Position(x, y)) != nullptr)
						continue;

					// ensure that by Dijkstra, this cell belongs to coreMin
					size_t minDistance = 999999;
					int minDistanceCoreId;
					for (auto & core : coreDistanceMaps)
					{
						size_t distance = coreDistanceMaps[core.first][y][x];
						if (distance < minDistance)
						{
							minDistance = distance;
							minDistanceCoreId = core.first;
						}
					}

					if (minDistanceCoreId == coreMinId)
					{
						newPos = Position(x, y);
						found = true;
					}
				}
			}
			if (!found)
			{
				// TODO: Keep relaxing the constraint by 1 until we find a position.
				continue;
			}

			Position oldPos = game->getObjects()[resourceToMove]->getPosition();
			game->getObjects()[resourceToMove]->setPosition(newPos);
			if (!areAllCoresConnected(game))
				game->getObjects()[resourceToMove]->setPosition(oldPos);

			coreFreq[coreMinId][d]++;
			coreResources[coreMinId].push_back(resourceToMove);
			resourceDistance[resourceToMove] = d;
			
			break;
		}
		if (balanced)
			break;
		++iter;
	}
	
	if (iter >= MAX_ITER)
		Logger::Log(LogLevel::WARNING, "Fair balancing did not converge fully after " + std::to_string(MAX_ITER) + " iterations.");
	else
		Logger::Log(LogLevel::INFO, "Fair resource balancing complete in " + std::to_string(iter) + " iterations.");
}
