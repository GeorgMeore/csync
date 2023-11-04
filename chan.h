#ifndef CHAN_INCLUDED
#define CHAN_INCLUDED

#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	size_t  objsize;
	void    *data;
	sem_t   cansend;
	sem_t   wantsend, wantrecv;
} chan;

#define CHAN_OF(type) chan

void chrecv(chan *ch, void *val)
{
	sem_post(&ch->wantrecv);
	sem_wait(&ch->wantsend);
	memcpy(val, ch->data, ch->objsize);
	sem_post(&ch->cansend);
}

int chtryrecv(chan *ch, void *val)
{
	if (sem_trywait(&ch->wantsend)) {
		return 0;
	}
	sem_post(&ch->wantrecv);
	memcpy(val, ch->data, ch->objsize);
	sem_post(&ch->cansend);
	return 1;
}

void chsend(chan *ch, void *val)
{
	sem_wait(&ch->cansend);
	memcpy(ch->data, val, ch->objsize);
	sem_post(&ch->wantsend);
	sem_wait(&ch->wantrecv);
}

int chtrysend(chan *ch, void *val)
{
	if (sem_trywait(&ch->cansend)) {
		return 0;
	}
	if (sem_trywait(&ch->wantrecv)) {
		sem_post(&ch->cansend);
		return 0;
	}
	memcpy(ch->data, val, ch->objsize);
	sem_post(&ch->wantsend);
	return 1;
}

int chinit(chan *ch, size_t objsize)
{
	ch->data = calloc(objsize, 1);
	if (!ch->data)
		goto calloc_failed;
	ch->objsize = objsize;
	if (sem_init(&ch->cansend, 0, 1))
		goto cansend_failed;
	if (sem_init(&ch->wantsend, 0, 0))
		goto wantsend_failed;
	if (sem_init(&ch->wantrecv, 0, 0))
		goto wantrecv_failed;
	return 1;
wantrecv_failed:
	sem_destroy(&ch->wantsend);
wantsend_failed:
	sem_destroy(&ch->cansend);
cansend_failed:
	free(ch->data);
calloc_failed:
	return 0;
}

#define CHAN_INIT(chan, type) chinit(&chan, sizeof(type))

void chdestroy(chan *ch)
{
	sem_destroy(&ch->cansend);
	sem_destroy(&ch->wantsend);
	sem_destroy(&ch->wantrecv);
	free(ch->data);
}

/* TODO: chclose */

#endif /* CHAN_INCLUDED */
