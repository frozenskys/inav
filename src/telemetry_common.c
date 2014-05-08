#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "platform.h"

#include "drivers/gpio_common.h"
#include "drivers/timer_common.h"
#include "drivers/serial_common.h"
#include "drivers/serial_softserial.h"
#include "serial_common.h"

#include "runtime_config.h"
#include "config.h"

#include "telemetry_common.h"
#include "telemetry_frsky.h"
#include "telemetry_hott.h"


static bool isTelemetryConfigurationValid = false; // flag used to avoid repeated configuration checks

static telemetryConfig_t *telemetryConfig;

void useTelemetryConfig(telemetryConfig_t *telemetryConfigToUse)
{
    telemetryConfig = telemetryConfigToUse;
}

bool isTelemetryProviderFrSky(void)
{
    return telemetryConfig->telemetry_provider == TELEMETRY_PROVIDER_FRSKY;
}

bool isTelemetryProviderHoTT(void)
{
    return telemetryConfig->telemetry_provider == TELEMETRY_PROVIDER_HOTT;
}

bool canUseTelemetryWithCurrentConfiguration(void)
{
    if (!feature(FEATURE_TELEMETRY)) {
        return false;
    }

   if (!canOpenSerialPort(FUNCTION_TELEMETRY)) {
       return false;
   }

    return true;
}

void initTelemetry()
{

    isTelemetryConfigurationValid = canUseTelemetryWithCurrentConfiguration();

    checkTelemetryState();
}

static bool telemetryEnabled = false;

bool determineNewTelemetryEnabledState(void)
{
    bool telemetryPortIsShared;
    bool enabled = true;

    serialPortFunction_t *function = findSerialPortFunction(FUNCTION_TELEMETRY);
    telemetryPortIsShared = function && function->scenario != SCENARIO_TELEMETRY_ONLY;

    if (telemetryPortIsShared) {
        if (telemetryConfig->telemetry_switch)
            enabled = rcOptions[BOXTELEMETRY];
        else
            enabled = f.ARMED;
    }

    return enabled;
}

bool shouldChangeTelemetryStateNow(bool newState)
{
    return newState != telemetryEnabled;
}

static void configureTelemetryPort(void)
{
    if (isTelemetryProviderFrSky()) {
        configureFrSkyTelemetryPort(telemetryConfig);
    }

    if (isTelemetryProviderHoTT()) {
        configureHoTTTelemetryPort(telemetryConfig);
    }
}

void freeTelemetryPort(void)
{
    if (isTelemetryProviderFrSky()) {
        freeFrSkyTelemetryPort();
    }

    if (isTelemetryProviderHoTT()) {
        freeHoTTTelemetryPort();
    }
}

void checkTelemetryState(void)
{
    if (!isTelemetryConfigurationValid) {
        return;
    }

    bool newEnabledState = determineNewTelemetryEnabledState();

    if (!shouldChangeTelemetryStateNow(newEnabledState)) {
        return;
    }

    if (newEnabledState)
        configureTelemetryPort();
    else
        freeTelemetryPort();

    telemetryEnabled = newEnabledState;
}

void handleTelemetry(void)
{
    if (!isTelemetryConfigurationValid || !determineNewTelemetryEnabledState())
        return;

    if (isTelemetryProviderFrSky()) {
        handleFrSkyTelemetry();
    }

    if (isTelemetryProviderHoTT()) {
        handleHoTTTelemetry();
    }
}
