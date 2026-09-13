#pragma once

#include <utils/types.h>

typedef struct PIDControl PIDControl;
typedef PIDControl* PidControl;

#define PID_INTEGRATION_EULER   1
#define PID_INTEGRATION_TRAP    2

#ifndef PID_INTEGRATION
#define PID_INTEGRATION PID_INTEGRATION_EULER
#endif

/**
 * @brief Create and initialize PID
 *
 * @param K_p Proportional constant
 * @param K_i Integral constant
 * @param K_d Derivative constant
 * @param r Reference value
 * @param y Measured value
 * @param u Input value (PID output)
 * @return Pointer to PIDControl, NULL on fail
 */
PidControl pid_init(double K_p, double K_i, double K_d,
        double* r, double* y, double *u);

/**
 * @brief Destroy PID
 *
 * @param pid Pointer to PIDControl
 */
void pid_destroy(PidControl pid);

/**
 * @brief Compute PID process
 *
 * @param pid Pointer to PIDControl
 * @param dt Time interval
 */
void pid_compute(PidControl pid, double dt);
