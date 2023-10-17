// #include "spl0601.h"
// #include "hx3203.h"
#include "gl_app.h"
#include "gl_pwm.h"
#include "app_task.h"

/*For operating clusters and attributes*/
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>

/*For schedule?*/
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <system/SystemError.h>

LOG_MODULE_REGISTER(gl_app, CONFIG_MATTER_LOG_LEVEL);

k_timer glAppTimer;
bool sensor_hx3203_update_flag = 0;
uint32_t value_hx3203;

void testAPP()
{
    LOG_INF("testAPP function.\n");
    GLApp::Instance().initGLAppTimer();

    /*Binding to glPwmDevice*/
    GL_PwmDevice::Instance().bindingPWMDeviceToGLPwmDevice(AppTask::Instance().GetPWMDevice().GetPwmDtSpec());
    GL_PwmDevice::Instance().initGLPwmDeviceTimer();
}

void sensorEvent(const AppEvent &)
{
    value_hx3203 = (uint32_t)app_sensor_fetch("hx3203@44");
	LOG_INF("Sensor Measure Handler. Sensor:%d\n", value_hx3203);
	uint16_t value_set = (uint16_t)value_hx3203;
    
    // using namespace ::chip;
    // using namespace ::chip::app::Clusters;
	// EmberAfStatus status = IlluminanceMeasurement::Attributes::MeasuredValue::Set(1, value_set);
    // LOG_INF("[%d]Set attribute value to %d.\n",status, value_set);

    // using namespace ::chip::DeviceLayer;
    // SystemLayer().ScheduleLambda([]{
	// 	/* write the new on/off value */
    //     uint16_t sValue = (uint16_t)value_hx3203;
	// 	EmberAfStatus status =
	// 		chip::app::Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Set(1, sValue);

	// 	if (status != EMBER_ZCL_STATUS_SUCCESS) {
	// 		LOG_ERR("Updating on/off cluster failed: %x", status);
	// 	}
	// });
}

double app_sensor_fetch(const char * name)
{
    /*Test getting sensor data*/
    const struct device *dev ;
    dev = device_get_binding(name);
    struct sensor_value value;
    int err ;

    err = sensor_sample_fetch(dev);
    if (err < 0) {
        return err;
    }
    
    err = sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &value);
    if (err < 0) {
        return err;
    }

    // LOG_INF("\n[%s]:%10.6f\n", name, sensor_value_to_double(&value));
    return sensor_value_to_double(&value);
}

void GLApp::initGLAppTimer()
{
    /* Initialize function timer */
    k_timer_init(&glAppTimer, &GLApp::autoAdjustPWMLevel, nullptr);
    k_timer_user_data_set(&glAppTimer, this);
    k_timer_start(&glAppTimer, K_MSEC(500), K_MSEC(500));  //simple start
}

void GLApp::autoAdjustPWMLevel(k_timer *timer)
{
#if 1   //sensor value update Test
    sensor_hx3203_update_flag = true;

    uint32_t autoBrightness = 0;
    autoBrightness = 1000 - ((value_hx3203 >= 1000) ? 1000 : value_hx3203);  //simple calculate
    // AppTask::Instance().GetPWMDevice().InitiateAction(
    //         PWMDevice::LEVEL_ACTION, static_cast<int32_t>(AppEventType::Lighting), &pwmLevel);
    GL_PwmDevice::Instance().glPwmDeviceMoveToDutyMilli(autoBrightness, 200);
    LOG_INF("sensor value:%d, Move to autoBrightness %d", value_hx3203, autoBrightness);

    //Get attribute test
    // chip::app::DataModel::Nullable<uint16_t> value_get;
    // chip::app::Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Get(1, value_get);
    // LOG_INF("Get attribute:%d",value_get.Value());

    //Can not work normally.
    // uint16_t value_set = (uint16_t)value_hx3203;
    // chip::EndpointId epId = 1;
    // chip::app::Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Set(epId, value_set);

    // using namespace ::chip::DeviceLayer;
    // SystemLayer().ScheduleLambda([this]{
	// 	/* write the new on/off value */
    //     uint16_t sValue = (uint16_t)value_hx3203;
    //     chip::EndpointId epId = 1;
	// 	EmberAfStatus status =
	// 		chip::app::Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Set(epId, sValue);

	// 	if (status != EMBER_ZCL_STATUS_SUCCESS) {
	// 		LOG_ERR("Updating on/off cluster failed: %x", status);
	// 	}
	// });

    // AppEvent event;
    // event.Type = AppEventType::SensorMeasure;
    // event.Handler = sensorEvent;
    // AppTask::Instance().PostEvent(event);

#endif

#if 0   //LED Loop Breathing Test
    //Get clock frequency
    // uint64_t cycles_get = 0;
    // pwm_get_cycles_per_sec(pwmSpec->dev, pwmSpec->channel, &cycles_get);
    // LOG_INF("Get cycles:%d", cycles_get);

    const struct pwm_dt_spec *pwmSpec = AppTask::Instance().GetPWMDevice().GetPwmDtSpec();
    static uint32_t loopValue = 0;
    static bool dir = 1;
    pwm_set_pulse_dt(pwmSpec, loopValue);
    loopValue = dir ? (loopValue + pwmSpec->period/1000) : (loopValue - pwmSpec->period/1000);
    if(loopValue >= pwmSpec->period)
    {
        dir = 0;
        LOG_INF("\nLight brightness change invert to 0.\n");
    }else if(loopValue <= 0)
    {
        dir = 1;
        LOG_INF("\nLight brightness change invert to 1.\n");
    }
#endif
}


