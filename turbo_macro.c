#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <xdo.h>
#include <X11/Xlib.h>
#include <assert.h>
#include <sys/time.h>

xdo_t *xdo_ins;

int key_down[10];


#define RELEASED 0
#define PRESSED 1
#define REPEATED 2

#define FREQ 10

int main(void) {
  // -------------------- Initialize
  const char *dev = "/dev/input/by-path/pci-0000:00:1a.0-usb-0:1.2:1.0-event-kbd";
  struct input_event ev;
  ssize_t n;
  int fd;
  int i;

  xdo_ins = xdo_new(NULL);

  for (i = 0; i < 10; i++) {
    key_down[i] = 0;
  }

  fd = open(dev, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
    return EXIT_FAILURE;
  }

  struct timeval tvLast, tvCur;
  gettimeofday(&tvLast, NULL);
  gettimeofday(&tvCur, NULL);

  // -------------------- Update
  while (1) {
    gettimeofday(&tvCur, NULL);
    unsigned long micros_delta = tvCur.tv_usec - tvLast.tv_usec;

    if (micros_delta > 1000000 / FREQ) {
      //printf("%d\n", micros_delta);
      tvLast = tvCur;
      for (i = 0; i < 10; i++) {
	if (key_down[i]) {
	  char keystr[8];
	  keystr[0] = '\0';
	  sprintf(keystr, "%d", (i + 1) % 10);
	  xdo_send_keysequence_window(xdo_ins, 0, keystr, 0);
	}
      }
    }

    usleep(100);

    int flags = fcntl(fd, F_GETFL, 0);
    assert(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
    n = read(fd, &ev, sizeof ev);
    if (n == (ssize_t)-1) {
      if (errno == EINTR)
	continue;
      //else
      //puts("not EINTR");
      //break;
    } else if (n != sizeof ev) {
      errno = EIO;
      break;
    }
    if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2) {
      unsigned int key = ev.code - 1;
      //printf("%d\n", key);
      if (key >= 1 && key <= 10) {
	switch (ev.value) {
	case RELEASED: {
	  key_down[key - 1] = 0;
	  break;
	}
	case PRESSED: {
	  key_down[key - 1] = 1;
	  break;
	}
	case REPEATED: {
	  
	  break;
	}

	default:
	  break;
	}
      }
    }
  }

  xdo_free(xdo_ins);
  //XCloseDisplay(display);

  fflush(stdout);
  fprintf(stderr, "%s.\n", strerror(errno));
  return EXIT_FAILURE;
}
