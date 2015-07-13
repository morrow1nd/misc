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

extern int schem_to_tpl(char *buf, size_t len, UT_vector *records, char *outfile);

void usage(char *exe) {
  fprintf(stderr,"usage: %s [options] [-t <out.tpl>] <file.nbt>\n", exe);
  fprintf(stderr,"          -v  (verbose- tag parsing)\n");
  fprintf(stderr,"          -vv (verbose- see records)\n");
  fprintf(stderr,"          -z  (unzip to stdout)\n");
  exit(-1);
}

void dump_records(UT_vector *records) {
  struct nbt_record *r=NULL;
  while ( (r = (struct nbt_record*)utvector_next(records,r))) {
    printf("%s [tag %.*s type:%u pos:%lu count:%u]\n", 
      utstring_body(&r->fqname), r->tag.len, r->tag.name, 
      (int)r->tag.type, (long)r->pos, r->count);
  }
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
  char *file=NULL, *in, *unz=NULL, *tpl=NULL;
  UT_vector *records=NULL;

  while ( (opt = getopt(argc,argv,"vhzt:")) > 0) {
    switch(opt) {
      case 'v': verbose++; break;
      case 'z': zcat = 1; break;
      case 't': tpl = strdup(optarg); break;
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

  if (verbose > 1) dump_records(records);
  if (tpl && (schem_to_tpl(unz, ulen, records, tpl) < 0)) goto done;

  rc = 0;

 done:
  if (records) utvector_free(records);
  if (in) free(in);
  if (unz) free(unz);
  if (tpl) free(tpl);
  return rc;
}
