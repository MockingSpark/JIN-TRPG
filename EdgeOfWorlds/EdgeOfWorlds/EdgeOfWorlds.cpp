// EdgeOfWorlds.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../GameEngine/GameEngine.h"


int main()
{
	GameEngine g;
    int end = g.run();
	return end;
}

