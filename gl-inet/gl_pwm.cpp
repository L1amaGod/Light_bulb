#include "gl_pwm.h"

LOG_MODULE_REGISTER(gl_pwm, CONFIG_MATTER_LOG_LEVEL);

void GL_PwmDevice::initGLPwmDeviceTimer()
{
    /* Initialize function timer */
    k_timer_init(&glPwmDeviceTimer, &GL_PwmDevice::glPwmDeviceTimerHandler, nullptr);
    k_timer_user_data_set(&glPwmDeviceTimer, this);
}

void GL_PwmDevice::startGLPwmDeviceTimer(uint16_t delayStartMs, uint16_t periodMs)
{
    k_timer_start(&glPwmDeviceTimer, K_MSEC(delayStartMs), K_MSEC(periodMs));  
}

void GL_PwmDevice::stopGLPwmDeviceTimer()
{
    k_timer_stop(&glPwmDeviceTimer);
}

void GL_PwmDevice::computePwmFadeInfo(uint16_t currentPwmDutyMilli, uint16_t targetPwmDutyMilli, uint16_t fadeTime)
{
    if(fadeTime == 0){
        fadeTime = 1;   //Limit min fade time
    }

    //Limit max pwm Value
    if(currentPwmDutyMilli > 1000){ //Use 1000 as max temporarily
        currentPwmDutyMilli = 1000;   //Use 1000 as max temporarily
    }
    if(targetPwmDutyMilli > 1000){ //Use 1000 as max temporarily
        targetPwmDutyMilli = 1000;    //Use 1000 as max temporarily
    }

    uint16_t changePwmDutyMilli;
    fInfo.currentPwmDutyMilli = currentPwmDutyMilli;
    fInfo.targetPwmDutyMilli = targetPwmDutyMilli;
    if(targetPwmDutyMilli - currentPwmDutyMilli >= 0)
    {
        changePwmDutyMilli = targetPwmDutyMilli - currentPwmDutyMilli;
        fInfo.direction = 1;  //1 means UP
    }
    else
    {
        changePwmDutyMilli = (currentPwmDutyMilli - targetPwmDutyMilli);
        fInfo.direction = 0;  //1 means DOWN
    }

    if(changePwmDutyMilli >= fadeTime)
    {
        fInfo.stepPwmDutyMilli_phase2 = changePwmDutyMilli / fadeTime;
        fInfo.remainingTime = changePwmDutyMilli - fInfo.stepPwmDutyMilli_phase2 * fadeTime;   //phase 1 time
        fInfo.remainingTime_phase2 = fadeTime - fInfo.remainingTime;
        fInfo.stepPwmDutyMilli_phase1 = fInfo.stepPwmDutyMilli_phase2 + 1;

        fInfo.stepPwmDutyMilli = fInfo.stepPwmDutyMilli_phase1;     //Start step from phase 1
        fInfo.lastStepValue = fInfo.stepPwmDutyMilli_phase2;   //End at phase 2

        //If > fadeTime, will not need counter.
        fInfo.countValue = 0;
        fInfo.counter = 0;
        fInfo.countTimes = 0;
    }
    else if((changePwmDutyMilli < fadeTime) && (changePwmDutyMilli != 0))
    {
        fInfo.stepPwmDutyMilli = 1;
        fInfo.countValue = fadeTime / changePwmDutyMilli;
        fInfo.counter = fInfo.countValue;
        fInfo.countTimes = changePwmDutyMilli / fInfo.stepPwmDutyMilli;  // countTimes = changePwmDutyMilli
        fInfo.remainingTime = fInfo.countTimes * fInfo.countValue;
        fInfo.lastStepValue = 0;
        fInfo.stepPwmDutyMilli_phase1 = 0;
        fInfo.stepPwmDutyMilli_phase2 = 0;
        fInfo.remainingTime_phase2 = 0;
    }
    else if(changePwmDutyMilli == 0)
    {
        fInfo.stepPwmDutyMilli = 0;
        fInfo.remainingTime = 0;
        fInfo.lastStepValue = 0;
        fInfo.countValue = 0;
        fInfo.counter = 0;
        fInfo.countTimes = 0;
    }
    // LOG_INF("\n 0x%x:Calculate Result:", this);
    // LOG_INF("start:%d, step:%d, tar:%d, dir:%d", currentPwmDutyMilli, fInfo.stepPwmDutyMilli, targetPwmDutyMilli, fInfo.direction);
}

void GL_PwmDevice::glPwmDeviceTimerHandler(k_timer *timer)
{
    if(GL_PwmDevice::Instance().startDimmingFlag)
    {
        fadeProcessInfo & info = GL_PwmDevice::Instance().fInfo;
        if(info.remainingTime > 0)
        {
            info.remainingTime --;
            //If changeValue < fadeTime, will do a rough counting to make real fadeTime be closed to the setting value
            if(info.counter > 1)
            {
                info.counter --;
                return;
            }
            else
            {
                if(info.countTimes > 0)
                {
                    info.countTimes --;
                    info.counter = info.countValue;
                }

                uint16_t curValue = GL_PwmDevice::Instance().glPwmDeviceGetPwmDutyMilli();
                info.currentPwmDutyMilli = curValue;
                uint16_t nextValue;
                nextValue = info.direction ? (curValue + info.stepPwmDutyMilli) : (curValue - info.stepPwmDutyMilli);
                // LOG_INF("current:%d, target:%d, remainingTime:%d, be going to next:%d", curValue, info.targetPwmDutyMilli, info.remainingTime, nextValue);
                GL_PwmDevice::Instance().glPwmDeviceSetPwmDutyMilli(nextValue);
                if(info.remainingTime == 0)
                {
                    if(info.remainingTime_phase2 > 0)   //switch to phase 2
                    {
                        info.remainingTime = info.remainingTime_phase2 - 1; //can not change or delete this.
                        info.stepPwmDutyMilli = info.stepPwmDutyMilli_phase2;
                        info.remainingTime_phase2 = 0;
                    }
                }
            }
        }
        else
        {
            //The last step
            if(info.remainingTime == 0)
            {
                info.counter = 0;
                info.countValue = 0;
                info.countTimes = 0;
                uint16_t curValue = GL_PwmDevice::Instance().glPwmDeviceGetPwmDutyMilli();
                info.currentPwmDutyMilli = curValue;
                uint16_t nextValue;
                nextValue = info.direction ? (curValue + info.lastStepValue) : (curValue - info.lastStepValue);
                LOG_INF("Last step:%d ,final value:%d, target:%d, dir:%d", info.lastStepValue, nextValue, info.targetPwmDutyMilli, info.direction);
                GL_PwmDevice::Instance().glPwmDeviceSetPwmDutyMilli(nextValue);
                GL_PwmDevice::Instance().stopGLPwmDeviceTimer();   //it can't seem to stop in time, which will cause this part over run.
                GL_PwmDevice::Instance().startDimmingFlag = 0;
            }
        }
    }else if(GL_PwmDevice::Instance().startDimmingFlag == 0){
        GL_PwmDevice::Instance().stopGLPwmDeviceTimer();
    }
}

void GL_PwmDevice::glPwmDeviceMoveToDutyMilli(uint16_t targetPwmDutyMilli, uint16_t fadeTime)
{
    uint16_t currentPwmDutyMilli = glPwmDeviceGetPwmDutyMilli();
    computePwmFadeInfo(currentPwmDutyMilli, targetPwmDutyMilli, fadeTime);
    LOG_INF("current:%d, target:%d, fadeTime:%d", currentPwmDutyMilli, targetPwmDutyMilli, fadeTime);
    if(currentPwmDutyMilli != targetPwmDutyMilli)   //Save some cpu resources
    {
        startDimmingFlag = 1;
        LOG_INF("\nStart fading timer.\n");
        startGLPwmDeviceTimer(10 , 1);
    }
}