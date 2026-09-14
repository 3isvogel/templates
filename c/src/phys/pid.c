#include "pid.h"
#include <stdlib.h>

struct PIDControl {
    double K_p, K_i, K_d;
    double *r, *y, *u;
    double prev, acc;
    double min, max;
};

PidControl pid_init(double K_p, double K_i, double K_d,
        double min, double max,
        double* r, double* y, double *u)
{
    PidControl pid = (PidControl)malloc(sizeof(PIDControl));
    *pid = (PIDControl) {
        .K_p = K_p, .K_i = K_i, .K_d = K_d,
        .prev = 0, .acc = 0,
        .r  = r,
        .y  = y,
        .u  = u,
        .min = min,
        .max = max,
    };
    return pid;
}

void pid_destroy(PidControl pid)
{
    free(pid);
}

#define DEREF(x) (*(x))
void pid_compute(PidControl pid, double dt)
{
    const double    r       = DEREF(pid->r),
                    y       = DEREF(pid->y),
                    min     = pid->min,
                    max     = pid->max,
                    prev    = pid->prev;

    // x_ref(t) - y(t)
    const double e = r - y;
    #if PID_INTEGRATION == PID_INTEGRATION_EULER
    // Euler integration
    // y(n) = y(n-1) + dt * u(t)
    pid->acc += dt * e;
    #elif PID_INTEGRATION == PID_INTEGRATION_TRAP
    // Trapezoidal integration
    // y(n) = y(n-1) + dt * [u(n) + u(n-1)]/2
    pid->acc += dt * (e + prev)/2;
    #else
    #error "Unknown value for PID_INTEGRATION, select one from PID_INTEGRATION_*"
    #endif

    const double p = e,
                 i = pid->acc,
                 d = (e - prev) / dt;
    // Update derivator
    pid->prev = e;

    const double u = pid->K_p * p + pid->K_i * i + pid->K_d * d;

    if (u < min) {
        *pid->u = min;
        #if PID_INTEGRATION_ANTI_WINDUP != 0
        pid->acc = 0;
        #endif
    } else if (u > max) {
        *pid->u = max;
        #if PID_INTEGRATION_ANTI_WINDUP != 0
        pid->acc = 0;
        #endif
    } else {
        *pid->u = u;
    }
}
