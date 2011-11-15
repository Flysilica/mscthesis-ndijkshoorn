#include "global.h"
#include "bot_ardrone.h"
#include "bot_ardrone_keyboard.h"
#include "bot_ardrone_behavior.h"


/* global variables */
bool exit_application	= false;
bool stop_behavior		= false;


int main(int argc, char *argv[])
{
	HWND consoleWindow = GetConsoleWindow();
	MoveWindow(consoleWindow, 0, 0, 600, 400, false);


	int nr_bots = 0;
	bot_ardrone *bots[1];

	/* bot 1: USARSim ARDRONE */
	// deployment location set in bot_ardrone_usarsim.cpp (top)
	//bot_ardrone ardrone(0x00, BOT_ARDRONE_INTERFACE_USARSIM, SLAM_MODE_MAP | SLAM_MODE_VISUALMOTION | SLAM_MODE_VISUALLOC);
	bot_ardrone ardrone(0x01, BOT_ARDRONE_INTERFACE_ARDRONELIB, SLAM_MODE_MAP);
	//bot_ardrone ardrone(0x01, BOT_ARDRONE_INTERFACE_NONE, SLAM_MODE_MAP | SLAM_MODE_VISUALMOTION /*| SLAM_MODE_VISUALLOC*/);
	bots[nr_bots++] = &ardrone;

	//ardrone.set_record();
	//ardrone.set_slam(true);
	//ardrone.set_playback("021");
	//ardrone.get_slam()->visual_map.save_canvas();
	//ardrone.get_slam()->m_frame->descriptor_map_quality();


	//bot_ardrone_behavior autonomous(&ardrone);

	bot_ardrone_keyboard kb(bots, nr_bots);

	return 0;
}