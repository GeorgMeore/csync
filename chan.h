#ifndef CHAN_INCLUDED
#define CHAN_INCLUDED

#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/* An unbuffered channel */
typedef struct {
	size_t  valsize;
	void    *val;
	sem_t   cansend;
	sem_t   wantsend, wantrecv;
} chan;

/* This macro is intended to be used for documentational purposes
 * in function arguments */
#define CHAN_OF(type) chan

void chrecv(chan *ch, void *val)
{
	sem_post(&ch->wantrecv);
	sem_wait(&ch->wantsend);
	memcpy(val, ch->val, ch->valsize);
	sem_post(&ch->cansend);
}

int chtryrecv(chan *ch, void *val)
{
	if (sem_trywait(&ch->wantsend))
		return 0;
	sem_post(&ch->wantrecv);
	memcpy(val, ch->val, ch->valsize);
	sem_post(&ch->cansend);
	return 1;
}

void chsend(chan *ch, void *val)
{
	sem_wait(&ch->cansend);
	memcpy(ch->val, val, ch->valsize);
	sem_post(&ch->wantsend);
	sem_wait(&ch->wantrecv);
}

int chtrysend(chan *ch, void *val)
{
	if (sem_trywait(&ch->cansend))
		return 0;
	if (sem_trywait(&ch->wantrecv)) {
		sem_post(&ch->cansend);
		return 0;
	}
	memcpy(ch->val, val, ch->valsize);
	sem_post(&ch->wantsend);
	return 1;
}

int chinit(chan *ch, size_t valsize)
{
	ch->val = calloc(valsize, 1);
	if (!ch->val)
		return 0;
	ch->valsize = valsize;
	if (sem_init(&ch->cansend, 0, 1)) {
		free(ch->val);
		return 0;
	}
	if (sem_init(&ch->wantsend, 0, 0)) {
		sem_destroy(&ch->cansend);
		free(ch->val);
		return 0;
	}
	if (sem_init(&ch->wantrecv, 0, 0)) {
		sem_destroy(&ch->wantsend);
		sem_destroy(&ch->cansend);
		free(ch->val);
		return 0;
	}
	return 1;
}

#define CHAN_INIT(chan, type) chinit(&chan, sizeof(type))

void chdestroy(chan *ch)
{
	sem_destroy(&ch->cansend);
	sem_destroy(&ch->wantsend);
	sem_destroy(&ch->wantrecv);
	free(ch->val);
}

/* TODO: chclose */

#endif /* CHAN_INCLUDED */
