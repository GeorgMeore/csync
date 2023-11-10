#ifndef RENDEZVOUS_INCLUDED
#define RENDEZVOUS_INCLUDED

#include <semaphore.h>
#include <pthread.h>
#include <string.h>

typedef struct {
	pthread_spinlock_t rvlock;
	size_t             valsize;
	sem_t              wait, done;
	void               *vals[2];
} rendezvous;

/*
 * This macro is intended to be used for
 * documentational purposes in function arguments.
 */
#define RENDEZVOUS_OF(type) rendezvous

void rvexchange(rendezvous *rv, void *src, void *dst)
{
	pthread_spin_lock(&rv->rvlock);
	if (!rv->vals[0]) { /* first */
		rv->vals[0] = src;
		pthread_spin_unlock(&rv->rvlock);
		sem_wait(&rv->wait);
		/* the lock is now held by the second thread */
		memcpy(dst, rv->vals[1], rv->valsize);
		sem_post(&rv->done);
	} else { /* second */
		rv->vals[1] = src;
		sem_post(&rv->wait);
		memcpy(dst, rv->vals[0], rv->valsize);
		sem_wait(&rv->done);
		/* reset the rendezvous pointers */
		memset(rv->vals, 0, sizeof(rv->vals));
		pthread_spin_unlock(&rv->rvlock);
	}
}

int rvinit(rendezvous *rv, size_t valsize)
{
	rv->valsize = valsize;
	memset(rv->vals, 0, sizeof(rv->vals));
	if (pthread_spin_init(&rv->rvlock, 0))
		return 0;
	if (sem_init(&rv->wait, 0, 0)) {
		pthread_spin_destroy(&rv->rvlock);
		return 0;
	}
	if (sem_init(&rv->done, 0, 0)) {
		pthread_spin_destroy(&rv->rvlock);
		sem_destroy(&rv->wait);
		return 0;
	}
	return 1;
}

void rvdestroy(rendezvous *rv)
{
	pthread_spin_destroy(&rv->rvlock);
	sem_destroy(&rv->wait);
	sem_destroy(&rv->done);
}

#endif /* RENDEZVOUS_INCLUDED */
