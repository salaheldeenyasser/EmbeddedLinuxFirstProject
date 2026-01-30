#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <errno.h>

#define LED_DEV     "/dev/ledblk0"
#define SWITCH_DEV  "/dev/switchblk0"

static int fd_led = -1;
static int fd_sw  = -1;
static volatile int running = 1;

/* ioctl definitions (must match drivers) */
#define LEDBLK_IOC_MAGIC   'L'
#define LEDBLK_CLEAR_BUF   _IO(LEDBLK_IOC_MAGIC, 0)

static void sigint_handler(int sig)
{
	(void)sig;
	running = 0;
}

static void cleanup(void)
{
	if (fd_led >= 0)
		close(fd_led);
	if (fd_sw >= 0)
		close(fd_sw);
}

int main(void)
{
	char sw;
	ssize_t ret;

	signal(SIGINT, sigint_handler);

	fd_led = open(LED_DEV, O_WRONLY);
	if (fd_led < 0) {
		perror("open ledblk");
		goto err;
	}

	fd_sw = open(SWITCH_DEV, O_RDONLY);
	if (fd_sw < 0) {
		perror("open switchblk");
		goto err;
	}

	/* Clear LED buffer at startup */
	if (ioctl(fd_led, LEDBLK_CLEAR_BUF) < 0)
		perror("ioctl clear buffer");

	printf("demo_app running (Ctrl+C to exit)\n");

	while (running) {
		lseek(fd_sw, 0, SEEK_SET);

		ret = read(fd_sw, &sw, 1);
		if (ret < 0) {
			perror("read switch");
			break;
		}

		lseek(fd_led, 0, SEEK_SET);

		if (write(fd_led, &sw, 1) < 0) {
			perror("write led");
			break;
		}

		printf("switch=%c -> led=%c\n", sw, sw);
		sleep(1);
	}

err:
	cleanup();
	printf("demo_app exited\n");
	return 0;
}
