#include "global.h"
#include "slam.h"
#include "bot_ardrone.h"
#include "opencv_helpers.h"

#include <cv.hpp>

using namespace cv;


slam::slam(unsigned char mode, unsigned char bot_id):
	KF(12, 9, 0)
{
	running = false;
	KF_running = false;

	m_frame = NULL;
	m_sensor = NULL;
	m_ui = NULL;

	m_sensor_paused = false;

	this->_mode = mode;
	this->bot_id = bot_id;

	//run();
}


slam::~slam()
{
}


void slam::run()
{
	running = true;

	init_kf();


	/* modules */
	m_frame		= new slam_module_frame((slam*) this);
	m_sensor	= new slam_module_sensor((slam*) this);
	m_ui		= new slam_module_ui((slam*) this);


	/* events */
	event_sensor_resume = CreateEvent(NULL, false, false, (LPTSTR) "SLAM_SENSOR_RESUME");


	/* start threads */
	thread_process_frame	= CreateThread(NULL, 0, start_process_frame, (void*) this, 0, NULL);
	thread_process_sensor	= CreateThread(NULL, 0, start_process_sensor, (void*) this, 0, NULL);
	thread_ui				= CreateThread(NULL, 0, start_ui, (void*) this, 0, NULL);
}


void slam::init_kf()
{
	// F vector
	setIdentity(KF.transitionMatrix); // completed (T added) when measurement received and T is known


	//setIdentity(KF.processNoiseCov, Scalar::all(1e-5));
	float PNC[12] = {
		20.0f, 20.0f, 20.0f,
		10.0f, 10.0f, 10.0f,
		3.0f, 3.0f, 3.0f,
		0.3f, 0.3f, 0.3f
	};
	MatSetDiag(KF.processNoiseCov, PNC);


	//setIdentity(KF.errorCovPost, Scalar::all(1));
	float ECP[12] = {
		20.0f, 20.0f, 20.0f,
		10.0f, 10.0f, 10.0f,
		3.0f, 3.0f, 3.0f,
		0.3f, 0.3f, 0.3f
	};
	MatSetDiag(KF.errorCovPost, ECP);
}


void slam::update_transition_matrix(float difftime)
{
	for (int i = 0; i < 3; i++)
	{
		// position (p)
		KF.transitionMatrix.at<float>(i, 3+i) = difftime;
		KF.transitionMatrix.at<float>(i, 6+i) = 0.5f * difftime*difftime;
		// velocity (v)
		KF.transitionMatrix.at<float>(3+i, 6+i) = difftime;
	}
}


bool slam::mode(unsigned char mode)
{
	return ((this->_mode & mode) != 0x00);
}


void slam::on(unsigned char mode)
{
	this->_mode |= mode;
}


void slam::off(unsigned char mode)
{
	this->_mode &= ~mode;
}


float* slam::get_state()
{
	return (float*) KF.statePost.data;
}


void slam::add_input_frame(bot_ardrone_frame *f)
{
	if (!running)
		run();

	// frame module not enable: drop frame
	if (m_frame == NULL)
		delete f;

	// drop frame is queue enabled but not empty
	if (SLAM_USE_QUEUE && queue_frame.empty())
		queue_frame.push(f);
	else if(!SLAM_USE_QUEUE)
		m_frame->process(f);
	else
		delete f;


	/*
	if (m_frame->frame_counter == 0)
	{
		char tmp[30];

		for (int i = 1; i < 60; i++)
		{
			if (i >= 10)
				sprintf_s(tmp, 30, "dataset/001/0000%i.raw\0", i);
			else
				sprintf_s(tmp, 30, "dataset/001/00000%i.raw\0", i);

			printf("%s\n", tmp);

			add_input_framefile(tmp);
		}
	}
	*/
}


void slam::add_input_framefile(char *filename)
{
	bot_ardrone_frame *f = new bot_ardrone_frame;
	f->usarsim = true;
	f->data_size = 76036;

	ifstream frame_in(filename, ios::in | ios::binary);

	// data buffer
	frame_in.read(f->data, f->data_size);
	frame_in.close();

	m_frame->process(f);
	Sleep(2000);
}


void slam::add_input_sensor(bot_ardrone_measurement *m)
{
	if (!running)
		run();

	if (SLAM_USE_QUEUE)
		queue_sensor.push(m);
	else if(!SLAM_USE_QUEUE)
		m_sensor->process(m);
}


void slam::get_world_position(float *pos)
{
	pos[0] = KF.statePost.at<float>(0);
	pos[1] = KF.statePost.at<float>(1);
	pos[2] = KF.statePost.at<float>(2);
	pos[3] = KF.statePost.at<float>(9);
	pos[4] = KF.statePost.at<float>(10);
	pos[5] = KF.statePost.at<float>(11);
}


void slam::sensor_pause(double time)
{
	if (m_sensor_paused)
		return;

	m_sensor_paused_time	= time;
	m_sensor_paused			= true;

	ResetEvent(event_sensor_resume);
}


void slam::sensor_resume()
{
	m_sensor_paused			= false;

	SetEvent(event_sensor_resume);
}



/* threads */
static DWORD WINAPI start_process_frame(void* Param)
{
	slam* This = (slam*) Param; 
	slam_queue<bot_ardrone_frame*> *q = &This->queue_frame;
	bot_ardrone_frame *item;
	slam_module_frame *processor = This->m_frame;

	while (!exit_application)
	{
		if (q->empty())
			q->wait_until_filled(2000);

		// just in case
		if (q->empty())
			continue;

		item = q->front();

		processor->process(item);

		q->pop();

		/* free memory */
		delete item;
	}

	return 1;
}


static DWORD WINAPI start_process_sensor(void* Param)
{
	slam* This = (slam*) Param; 
	slam_queue<bot_ardrone_measurement*> *q = &This->queue_sensor;
	bot_ardrone_measurement *item;
	slam_module_sensor *processor = This->m_sensor;

	bool *paused			= &This->m_sensor_paused;
	double *paused_time		= &This->m_sensor_paused_time;

	while (!exit_application)
	{
		if (q->empty())
			q->wait_until_filled(2000);

		// just in case
		if (q->empty())
			continue;

		item = q->front();

		// paused
		if (*paused && item->time >= *paused_time)
			WaitForSingleObject(This->event_sensor_resume, 5000);

		processor->process(item);

		q->pop();

		/* free memory */
		delete item;
	}

	return 1;
}


static DWORD WINAPI start_ui(void* Param)
{
	slam* This = (slam*) Param; 
	slam_module_ui *processor = This->m_ui;

	while (!exit_application)
	{
		processor->update();
		Sleep(35);
	}

	//processor->display_canvas();

	return 1;
}