#include "global.h"
#include "bot_ardrone.h"
#include "bot_ardrone_usarsim.h"
#include "bot_ardrone_ardronelib.h"
#include "bot_ardrone_recorder.h"
#include "opencv_helpers.h"

using namespace std;

clock_t bot_ardrone::start_clock = 0;

#undef memset

bot_ardrone_control::bot_ardrone_control()
{
	memset(this, 0, sizeof(bot_ardrone_control));
	state = BOT_STATE_LANDED;
}


bot_ardrone_measurement::bot_ardrone_measurement()
{
	memset(this, 0, sizeof(bot_ardrone_measurement));
	time = bot_ardrone::get_clock();
	usarsim = false;
}


bot_ardrone_frame::bot_ardrone_frame()
{
	memset(this, 0, sizeof(bot_ardrone_frame));
	this->data			= new char[BOT_ARDRONBOT_EVENT_FRAME_BUFSIZE];
	this->data_start	= this->data;
}


bot_ardrone_frame::~bot_ardrone_frame()
{
	delete[] this->data_start;
}


bot_ardrone::bot_ardrone(unsigned char id, unsigned char botinterface, unsigned char slam_mode)
{
	start_clock		= clock();
	id				= id;
	i				= NULL;
	i_id			= botinterface;
	recorder		= NULL;
	record			= playback = false;
	battery			= NULL;
	slam_state		= false;

	flyto_vel		= 5000.0f;

	control_reset();



	/* INTERFACE */
	switch (botinterface)
	{
		case BOT_ARDRONE_INTERFACE_USARSIM:
			i = new bot_ardrone_usarsim((bot_ardrone*) this);
			break;

		case BOT_ARDRONE_INTERFACE_ARDRONELIB:
			i = new bot_ardrone_ardronelib((bot_ardrone*) this);
			break;

		default:
			printf("ARDRONE: NO INTERFACE USED\n");
	}

	if (i != NULL)
		i->init();


	/* SLAM */
	slamcontroller = new slam(slam_mode, id);
}


bot_ardrone::~bot_ardrone(void)
{
}


void bot_ardrone::control_set(int type, int opt, float val)
{
	switch(type)
	{
		case BOT_ARDRONE_Velocity:
			control.velocity[opt] = val;
			control.state = BOT_STATE_FLY;
			break;

		default:
			break;
	}
}


float bot_ardrone::control_get(int type, int opt)
{
	switch(type)
	{
		case BOT_ARDRONE_Velocity:
			return control.velocity[opt];

		default:
			return 0.0f;
	}
}


void bot_ardrone::control_update()
{
	control.time = get_clock();

	control_update(&control);
}


void bot_ardrone::control_update(bot_ardrone_control *c)
{
	if (PRINT_DEBUG)
		printf("%f - ARDRONE: control update!\n", c->time);

	if(record)
		recorder->record_control(&control);

	if (i != NULL)
		i->control_update((void*) &control);
}


void bot_ardrone::control_reset()
{
	control.velocity[BOT_ARDRONE_AltitudeVelocity] = 0.0;
	control.velocity[BOT_ARDRONE_LinearVelocity] = 0.0;
	control.velocity[BOT_ARDRONE_LateralVelocity] = 0.0;
	control.velocity[BOT_ARDRONE_RotationalVelocity] = 0.0;

	if (control.state == BOT_STATE_FLY)
		control.state = BOT_STATE_HOVER;
}


void bot_ardrone::take_off()
{
	i->take_off();
	control.state = BOT_STATE_HOVER;
}


void bot_ardrone::land()
{
	i->land();
	control.state = BOT_STATE_LANDED;
}


void bot_ardrone::recover(bool send)
{
	if (control.state == BOT_STATE_LANDED)
		i->recover(send);
}


void bot_ardrone::measurement_received(bot_ardrone_measurement *m)
{
	if (exit_application)
		return;

	if (PRINT_DEBUG)
		printf("%f - ARDRONE: measurement received!\n", m->time);

	// time since last frame
	double diffticks = ((double)clock() - last_measurement_time) / CLOCKS_PER_SEC;
	if (diffticks < BOT_ARDRONE_MIN_MEASUREMENT_INTERVAL)
	{
		delete m;
		return;
	}

	last_measurement_time = clock();


	if (record)
		recorder->record_measurement(m);

	if (slam_state)
		slamcontroller->add_input_sensor(m);
	else
		delete m;
}


void bot_ardrone::frame_received(bot_ardrone_frame *f)
{
	if (exit_application)
		return;

	if (PRINT_DEBUG)
		printf("%f - ARDRONE: frame received: %s!\n", f->time, f->filename);

	if (record && BOT_ARDRONE_RECORD_FRAMES)
		recorder->record_frame(f);

	if (slam_state)
		slamcontroller->add_input_frame(f);
	else
		delete f;
}


double bot_ardrone::get_clock()
{
	return ((double)clock() - start_clock) / CLOCKS_PER_SEC;
}


void bot_ardrone::set_record()
{
	if (recorder == NULL)
	{
		recorder = new bot_ardrone_recorder((bot_ardrone*) this);
		recorder->prepare_dataset();
		record = true;
	}
}


void bot_ardrone::set_playback(char *dataset)
{
	if (recorder == NULL)
	{
		recorder = new bot_ardrone_recorder((bot_ardrone*) this);
		recorder->playback(dataset);
		playback = true;
	}
}


slam* bot_ardrone::get_slam()
{
	return slamcontroller;
}


void bot_ardrone::set_slam(bool state)
{
	slam_state = state;

	printf("# SLAM STATE: %s\n", state ? "ENABLED" : "DISABLED");
}


void bot_ardrone::get_slam_pos(float *pos)
{
	if (!slam_state)
		return;

	memcpy_s(pos, 12, slamcontroller->get_state(), 12);
}


void bot_ardrone::get_slam_or(float *or)
{
	if (!slam_state)
		return;

	float *state = slamcontroller->get_state();
	memcpy_s(or, 12, &state[9], 12);
}


bool bot_ardrone::flyto(float x, float y, float z)
{
	float *state;
	float pos[3];
	float or[3];
	float dx, dy;
	float vx, vy;
	float d_vx, d_vy;
	bool reached = false;

	Mat Mpos(3, 1, CV_32F);
	Mat Mor(3, 1, CV_32F);
	Mat Mrot(3, 3, CV_32F);
	Mor = 0.0f;

	while (1)
	{
		get_slam_pos(pos);
		get_slam_or(or);

		dx = x - pos[0];
		dy = y - pos[1];

		state = slamcontroller->get_state();
		vx = state[3];
		vy = state[4];

		if (abs(dx) < 200.0f && abs(dy) < 200.0f)
		 {
			control_reset();
			control_update();
			printf("reached %f, %f!\n", x, y);
			break; 
		}

		d_vx = (dx - 0.4f * vx);
		d_vy = (dy - 0.4f * vy);

		d_vx = d_vx / flyto_vel;
		d_vy = d_vy / flyto_vel;

		/* convert world distance to distance relative to ARDrone's orientation */
		Mpos.at<float>(0) = d_vx;
		Mpos.at<float>(1) = d_vy;
		Mpos.at<float>(2) = 0.0f;

		Mor.at<float>(2) = -or[2];
		cv::RotationMatrix3D(Mor, Mrot);

		Mpos = Mrot * Mpos;
		vx = Mpos.at<float>(0);
		vy = Mpos.at<float>(1);

		if (vx < -1.0f)
			vx = -1.0f;
		if (vx > 1.0f)
			vx = 1.0f;
		if (vy < -1.0f)
			vy = -1.0f;
		if (vy > 1.0f)
			vy = 1.0f;

		control_set(BOT_ARDRONE_Velocity, BOT_ARDRONE_LinearVelocity, vx);
		control_set(BOT_ARDRONE_Velocity, BOT_ARDRONE_LateralVelocity, vy);
		control_update();

		//printf("vel: %f, %f\n", velX, velY);

		if (stop_behavior)
			return false;

		Sleep(50);
	}

	return true;
}


bool bot_ardrone::heightto(float z)
{
	float pos[3];
	float dz;
	bool reached = false;

	Mat Mpos(3, 1, CV_32F);

	while (1)
	{
		get_slam_pos(pos);

		dz = z - pos[2];

		if (abs(dz) < 200.0f)
		{
			control_reset();
			control_update();
			printf("reached alt %f!\n", z);
			break; 
		}
		else
		{
			//printf("pos: %f, %f dist: %f, %f OR: %f\n", pos[0], pos[1], dx, dy, or[2]);
		}

		float velZ = min(0.7f, (dz*dz) / 1000000.0f);

		if (dz > 0.0f)
			velZ = -velZ;

		control_set(BOT_ARDRONE_Velocity, BOT_ARDRONE_AltitudeVelocity, velZ);
		control_update();

		//printf("vel: %f, %f\n", velX, velY);

		if (stop_behavior)
			return false;

		Sleep(50);
	}

	return true;
}