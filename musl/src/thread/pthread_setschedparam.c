#include "pthread_impl.h"

int pthread_setschedparam(pthread_t t, int policy, const struct sched_param *param)
{
	int r;
	LOCK(t->killlock);
	r = t->dead ? ESRCH : -__syscall(SYS_sched_setscheduler, t->tid, policy, param);
	UNLOCK(t->killlock);
	return r;
}
