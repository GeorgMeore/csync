#ifndef CHAN_INCLUDED
#define CHAN_INCLUDED

#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define CHAN_OK     0
#define CHAN_CLOSED 1
#define CHAN_NOWAIT 2

/*
 * An unbuffered channel.
 *
 * All of the field are protected by a lock.
 * Procedures that operate on a channel must first obtain the lock.
 * Blocking procedures should release the lock before sleeping,
 * and an awakening procedure is expected to be holding it.
 */
typedef struct {
	pthread_spinlock_t chanlock;
	int                closed;
	int                rcnt,  wcnt;
	sem_t              rwait, wwait;
	sem_t              rdone, wdone;
	void               *src, *dst;
	size_t             valsize;
} chan;

/*
 * This macro is intended to be used for
 * documentational purposes in function arguments.
 */
#define CHAN_OF(type) chan

void chclose(chan *ch)
{
	pthread_spin_lock(&ch->chanlock);
	ch->closed = 1;
	pthread_spin_unlock(&ch->chanlock);
	sem_post(&ch->rwait);
	sem_post(&ch->wwait);
}

int chtrysend(chan *ch, void *val)
{
	pthread_spin_lock(&ch->chanlock);
	if (ch->closed) {
		pthread_spin_unlock(&ch->chanlock);
		return CHAN_CLOSED;
	}
	if (!ch->rcnt) {
		pthread_spin_unlock(&ch->chanlock);
		return CHAN_NOWAIT;
	}
	ch->src = val;
	sem_post(&ch->rwait);
	ch->rcnt -= 1;
	sem_wait(&ch->rdone);
	pthread_spin_unlock(&ch->chanlock);
	return CHAN_OK;
}

int chsend(chan *ch, void *val)
{
	pthread_spin_lock(&ch->chanlock);
	if (ch->closed) {
		pthread_spin_unlock(&ch->chanlock);
		return CHAN_CLOSED;
	}
	if (!ch->rcnt) {
		ch->wcnt += 1;
		pthread_spin_unlock(&ch->chanlock);
		sem_wait(&ch->wwait);
		/* at this point either the lock is held by the signaller
		 * or the channel is closed */
		if (ch->closed) {
			sem_post(&ch->wwait);
			return CHAN_CLOSED;
		}
		memcpy(ch->dst, val, ch->valsize);
		sem_post(&ch->wdone);
		return CHAN_OK;
	} else {
		ch->src = val;
		sem_post(&ch->rwait);
		ch->rcnt -= 1;
		sem_wait(&ch->rdone);
		pthread_spin_unlock(&ch->chanlock);
		return CHAN_OK;
	}
}

int chtryrecv(chan *ch, void *val)
{
	pthread_spin_lock(&ch->chanlock);
	if (ch->closed) {
		pthread_spin_unlock(&ch->chanlock);
		return CHAN_CLOSED;
	}
	if (!ch->wcnt) {
		pthread_spin_unlock(&ch->chanlock);
		return CHAN_NOWAIT;
	}
	ch->dst = val;
	sem_post(&ch->wwait);
	ch->wcnt -= 1;
	sem_wait(&ch->wdone);
	pthread_spin_unlock(&ch->chanlock);
	return CHAN_OK;
}

int chrecv(chan *ch, void *val)
{
	pthread_spin_lock(&ch->chanlock);
	if (ch->closed) {
		pthread_spin_unlock(&ch->chanlock);
		return CHAN_CLOSED;
	}
	if (!ch->wcnt) {
		ch->rcnt += 1;
		pthread_spin_unlock(&ch->chanlock);
		sem_wait(&ch->rwait);
		/* at this point either the lock is held by the signaller
		 * or the channel is closed */
		if (ch->closed) {
			sem_post(&ch->rwait);
			return CHAN_CLOSED;
		}
		memcpy(val, ch->src, ch->valsize);
		sem_post(&ch->rdone);
		return CHAN_OK;
	} else {
		ch->dst = val;
		sem_post(&ch->wwait);
		ch->wcnt -= 1;
		sem_wait(&ch->wdone);
		pthread_spin_unlock(&ch->chanlock);
		return CHAN_OK;
	}
}

int chinit(chan *ch, size_t valsize)
{
	ch->closed = 0;
	ch->valsize = valsize;
	ch->rcnt = ch->wcnt = 0;
	if (pthread_spin_init(&ch->chanlock, 0))
		return 0;
	if (sem_init(&ch->rdone, 0, 0))
		goto rdone_fail;
	if (sem_init(&ch->wdone, 0, 0))
		goto wdone_fail;
	if (sem_init(&ch->rwait, 0, 0))
		goto rwait_fail;
	if (sem_init(&ch->wwait, 0, 0))
		goto wwait_fail;
	return 1;
wwait_fail:
	sem_destroy(&ch->rwait);
rwait_fail:
	sem_destroy(&ch->wdone);
wdone_fail:
	sem_destroy(&ch->rdone);
rdone_fail:
	pthread_spin_destroy(&ch->chanlock);
	return 0;
}

/*
 * TODO: A select procedure.
 *
 * chan *chselect(chan *c1, int m1, void *v1, ..., NULL);
 *
 * This procedure should take a variadic number of triplets
 * (channel pointer, channel operation, value pointer).
 * These triplets must be followed by a NULL pointer.
 *
 * After that chselect should block until one of the operations
 * succeeds (in this case the channel should be returned)
 * or all of the channels are closed (in this case the procedure
 * should return NULL).
 *
 * Example usage:
 *
 * #define CHSELECT(args...) chselect(args..., NULL)
 *
 * ch = CHSELECT(
 *     c1, CHAN_READ,  &v1,
 *     c2, CHAN_WRITE, &v2,
 * )
 * if (ch == c1)
 *     printf("received a value from channel 1");
 * ...
 */

void chdestroy(chan *ch)
{
	sem_destroy(&ch->rwait);
	sem_destroy(&ch->wdone);
	sem_destroy(&ch->rdone);
	pthread_spin_destroy(&ch->chanlock);
}

#endif /* CHAN_INCLUDED */
