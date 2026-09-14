#pragma once

typedef struct PIDControl PIDControl;
typedef PIDControl* PidControl;

#define PID_INTEGRATION_EULER   1
#define PID_INTEGRATION_TRAP    2

// If not specified, use Euler integration
#ifndef PID_INTEGRATION
#define PID_INTEGRATION PID_INTEGRATION_EULER
#endif

// If not specified, do not enable anti-windup
// NOTE to tune PID enable anti-windup, once tuned disable it, as it causes minor
// error on the I component
#ifndef PID_INTEGRATION_ANTI_WINDUP
#define PID_INTEGRATION_ANTI_WINDUP 0
#endif

/**
 * @brief Create and initialize PID
 *
 * @param K_p Proportional constant
 * @param K_i Integral constant
 * @param K_d Derivative constant
 * @param min Minimum possible value of u
 * @param max Maximum possible value of u
 * @param r Reference value
 * @param y Measured value
 * @param u Input value (PID output)
 * @return Pointer to PIDControl, NULL on fail
 */
PidControl pid_init(double K_p, double K_i, double K_d,
        double min, double max,
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
