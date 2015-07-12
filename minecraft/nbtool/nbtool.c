#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "nbt.h"

int verbose;
int zcat; // dump unzipped data to stdout

void usage(char *exe) {
  fprintf(stderr,"usage: %s [options] <file>\n", exe);
  fprintf(stderr,"          -v  (verbose)\n");
  fprintf(stderr,"          -z  (unzip to stdout)\n");
  exit(-1);
}

char *slurp(char *file, size_t *flen) {
  struct stat stats;
  char *out=NULL;
  int fd=-1, rc=-1;

  if (stat(file, &stats) == -1) {
    fprintf(stderr, "can't stat %s: %s\n", file, strerror(errno));
    goto done;
  }
  *flen  = stats.st_size;
  if (flen == 0) {
    fprintf(stderr, "file %s is zero length\n", file);
    goto done;
  }
  if ( (out = malloc(stats.st_size)) == NULL) {
    fprintf(stderr, "can't malloc space for %s\n", file);
    goto done;
  }
  if ( (fd = open(file,O_RDONLY)) == -1) {
    fprintf(stderr, "can't open %s: %s\n", file, strerror(errno));
    goto done;
  }
  if ( read(fd, out, stats.st_size) != stats.st_size) {
    fprintf(stderr, "short read on %s\n", file);
    goto done;
  }

  rc = 0;

 done:
  if ((rc < 0) && (out != NULL)) { free(out); out = NULL; }
  if (fd != -1) close(fd);
  return out;
}

int main( int argc, char *argv[]) {
  int rc=-1, opt;
  size_t ilen, ulen;
  char *file=NULL, *in, *unz=NULL;
  UT_vector *records;

  while ( (opt = getopt(argc,argv,"vhz")) > 0) {
    switch(opt) {
      case 'v': verbose++; break;
      case 'z': zcat = 1; break;
      case 'h': default: usage(argv[0]); break;
    }
  }

  if (optind < argc) file = argv[optind++];
  if (file == NULL) usage(argv[0]);

  in = slurp(file, &ilen);
  if (in == NULL) goto done;

  rc = ungz(in, ilen, &unz, &ulen);
  if (rc) goto done;

  if (zcat) write(STDOUT_FILENO,unz,ulen);

  rc = parse_nbt(unz, ulen, &records, verbose);
  if (rc) goto done;

  printf("%u records\n", utvector_len(records));
  utvector_free(records);

  rc = 0;

 done:
  if (in) free(in);
  if (unz) free(unz);
  return rc;
}
