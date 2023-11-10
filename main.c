#include <stdio.h>
#include <unistd.h>

#include "coro.h"
#include "chan.h"
#include "rendezvous.h"

CORO_BEGIN1(printer, CHAN_OF(int) *in)
	int x;
	CORO_SETARGS1(in);
	for (;;) {
		int err = chrecv(in, &x);
		if (err == CHAN_CLOSED)
			break;
		printf("%d\n", x);
	}
	printf("printer finished\n");
CORO_END

CORO_BEGIN1(sender, CHAN_OF(int) *out)
	int x = 0;
	CORO_SETARGS1(out);
	for (;;) {
		int err = chsend(out, &x);
		if (err == CHAN_CLOSED)
			break;
		x += 1;
	}
	printf("sender finished\n");
CORO_END

int sendprinttest()
{
	chan c;
	if (!chinit(&c, sizeof(int)))
		return 1;
	CORO_START(sender, &c);
	CORO_START(printer, &c);
	getchar();
	chclose(&c);
	getchar();
	chdestroy(&c);
	return 0;
}

CORO_BEGIN2(foo, CHAN_OF(int) *in, CHAN_OF(int) *out)
	int x, y;
	CORO_SETARGS2(in, out);
	printf("starting a foo loop\n");
	for (;;) {
		int err = chtryrecv(in, &x);
		if (err == CHAN_CLOSED)
			break;
		if (err == CHAN_OK) {
			printf("foo received: %d\n", x);
			y = x + 10;
			chsend(out, &y);
		} else {
			printf("foo sleeping\n");
			sleep(1);
		}
	}
	printf("exited a foo loop\n");
	sleep(5);
	chclose(out);
CORO_END

int footest(void)
{
	chan in, out;
	int x, y;
	if (!chinit(&in, sizeof(int)) || !chinit(&out, sizeof(int)))
		return 1;
	CORO_START(foo, &in, &out);
	printf("starting the main loop\n");
	while (scanf(" %d", &x) == 1) {
		chsend(&in, &x);
		chrecv(&out, &y);
		printf("%d\n", y);
	}
	printf("exited the main loop\n");
	chclose(&in);
	chrecv(&out, &y);
	chdestroy(&in);
	chdestroy(&out);
	return 0;
}

CORO_BEGIN1(bar, RENDEZVOUS_OF(int) *rv)
	int x = 0, y;
	CORO_SETARGS1(rv);
	for (;;) {
		rvexchange(rv, &x, &y);
		if (!y)
			break;
		x = y * 10;
	}
CORO_END

int bartest(void)
{
	int x, y;
	rendezvous rv;
	if (!rvinit(&rv, sizeof(int)))
		return 1;
	CORO_START(bar, &rv);
	for (;;) {
		scanf(" %d", &x);
		rvexchange(&rv, &x, &y);
		printf("%d\n", y);
		if (!x)
			break;
	}
	rvdestroy(&rv);
	return 0;
}

int main(void)
{
	return bartest();
}
