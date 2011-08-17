// car_test.cpp : Defines the entry point for the console application.
//

#include <irrlicht.h>
#include <ode/ode.h>

#include "GameContext.h"

#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
#endif

using namespace irr;
using namespace core;
using namespace gui;

int main(int argc, char* argv[])
{
	// init Irrlicht
	IrrlichtDevice *device = createDevice(video::EDT_OPENGL, core::dimension2d<u32>(640,480), 16, false, false, false);
	if (!device)
		return 1;

	device->setWindowCaption(L"Cars and Walls");

	// init ODE
	dInitODE2(0);

	CGameContext * game = new CGameContext(device);

	game->mainMenu();
    
	while (device->run())
		if (device->isWindowActive())
    		game->gameStep();

	delete game;

	// destroy ODE
	dCloseODE();

	// destroy irricht
	device->drop();

	return 0;
}

