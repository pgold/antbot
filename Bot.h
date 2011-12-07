#ifndef BOT_H_
#define BOT_H_

#include "State.h"

/*
    This struct represents your bot in the game of Ants
*/
struct Bot
{
    State state;
	int *mark;
	int *pred;
	bool explored;

    Bot();

    void playGame();    //plays a single game of Ants

    void makeMoves();   //makes moves for a single turn
    void endTurn();     //indicates to the engine that it has made its moves

	int bfs(Location start, Location target);
	Location explorerBfs(Location ant);
	Location foodBfs(Location food);
	int nearbyBfs(Location start, int what, int dist);
	Location patrollerBfs(Location ant);
	std::vector<Location> spartanBfs(Location target);

	void move(Location location, int dir);
	void stay(Location location);

	bool isFood(Location loc);
	bool isWater(Location loc);
	bool isClear(Location loc);
	bool isVisible(Location loc);
	bool isFreeAnt(Location loc);
	bool isSeen(Location loc);

	Location north(int ant);
	Location east(int ant);
	Location south(int ant);
	Location west(int ant);

	int hillDist(int ant);
	Location nearestEnemy(Location loc);

	void schedDefenders();
	void schedExplorers();
	void schedFleers();
	void schedFoodFetchers();
	void schedGuardians();
	void schedPatrollers();
	void schedSpartans();

	// Modes
	void avoider(int ant);
	void clockWise(int ant);
	void explore(int ant);
	void firstFree(int ant);
	void randWalk(int ant);
};

#endif //BOT_H_
