#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include "app_event.h"
#include "pwm_device.h"

#include <platform/CHIPDeviceLayer.h>


typedef struct{
   uint16_t currentPwmDutyMilli;
   uint16_t targetPwmDutyMilli;
   uint16_t stepPwmDutyMilli;           //step value to run actually
   uint16_t stepPwmDutyMilli_phase1;    //fade phase 1: stepPwmDutyMilli + 1 (phase 1 is faster than phase 2)
   uint16_t stepPwmDutyMilli_phase2;    //fade phase 2: stepPwmDutyMilli
   uint16_t remainingTime_phase2;  //step time of phase 2
   uint16_t remainingTime;          //Time to run actually
   uint16_t lastStepValue;
   uint16_t counter;          //Variation, counter is for dimming while step value is less than transition time
   uint16_t countValue;       //Constant Value, get from calculation
   uint16_t countTimes;       //Constant Value, get from calculation
   bool direction;
}fadeProcessInfo;

class GL_PwmDevice : PWMDevice{
public:
	static GL_PwmDevice &Instance()
	{
		static GL_PwmDevice sGL_PwmDevice;
		return sGL_PwmDevice;
	};

    bool startDimmingFlag;
    const struct pwm_dt_spec *glPwmDeviceSpec;  //TODO:Maybe change to private?
    fadeProcessInfo fInfo;

    void bindingPWMDeviceToGLPwmDevice(const pwm_dt_spec *pwmSpec){glPwmDeviceSpec = pwmSpec;};
    const struct pwm_dt_spec * glPwmDevice(){return glPwmDeviceSpec;};
    void initGLPwmDeviceTimer();
    void startGLPwmDeviceTimer(uint16_t delayStartMs, uint16_t periodMs);
    void stopGLPwmDeviceTimer();

    void computePwmFadeInfo(uint16_t currentPwmDutyMilli, uint16_t targetPwmDutyMilli, uint16_t fadeTime);
    static void glPwmDeviceTimerHandler(k_timer *timer);
    uint16_t glPwmDeviceGetPwmDutyMilli(){return pulseWidthNs/(glPwmDeviceSpec->period/1000);};
    void glPwmDeviceSetPwmDutyMilli(uint16_t dutyMilli){
        pulseWidthNs = glPwmDeviceSpec->period/1000;
        pulseWidthNs *= dutyMilli;
        pwm_set_pulse_dt(glPwmDeviceSpec, pulseWidthNs); 
    };

    void glPwmDeviceMoveToDutyMilli(uint16_t targetPwmDutyMilli, uint16_t fadeTime);

private:
    k_timer glPwmDeviceTimer;
    uint32_t pulseWidthNs;
};