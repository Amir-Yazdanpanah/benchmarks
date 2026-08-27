#include "support.h"
#include "qrencode.h"
#include "beebsc.h"
#include <string.h>

#define LOCAL_SCALE_FACTOR 5
#define HEAP_SIZE 8192

static char heap[HEAP_SIZE];
static const char *encode;
static int size;

static int benchmark_body(unsigned int lsf, unsigned int gsf) {
  static const char *in_encode = "http://www.mageec.com";

  for (unsigned int lsf_cnt = 0; lsf_cnt < lsf; lsf_cnt++)
    for (unsigned int gsf_cnt = 0; gsf_cnt < gsf; gsf_cnt++) {
	encode = in_encode;
	size = 22;
	init_heap_beebs ((void *) heap, HEAP_SIZE);

	initeccsize (1, size);
	memcpy (strinbuf, encode, size);

	initframe ();
	qrencode ();
	freeframe ();
	freeecc ();
    }

  return 0;
}

int benchmark (void) {
  return benchmark_body (LOCAL_SCALE_FACTOR, GLOBAL_SCALE_FACTOR);
}

void initialise_benchmark () {}

int verify_benchmark (int unused) {
  (void)unused;
  unsigned char expected[22] = {
    254, 101, 63, 128, 130, 110, 160, 128, 186, 65, 46,
    128, 186, 38, 46, 128, 186, 9, 174, 128, 130, 20
  };

  return (0 == memcmp (strinbuf, expected, 22 * sizeof (strinbuf[0])))
    && check_heap_beebs ((void *) heap);
}