#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include "hx3203.h"
#include "spl0601.h"

#ifdef __cplusplus
extern "C"{
#endif

extern bool sensor_hx3203_update_flag;
extern uint32_t value_hx3203;

class GLApp{
public:
	static GLApp &Instance()
	{
		static GLApp sGLApp;
		return sGLApp;
	};

    void initGLAppTimer();
    static void autoAdjustPWMLevel(k_timer *timer);

	// void sensorEvent(const AppEvent &);  //Test
};


void testAPP();
double app_sensor_fetch(const char * name);

#ifdef __cplusplus
}
#endif