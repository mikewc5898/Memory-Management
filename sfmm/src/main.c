#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();
    double *a = sf_malloc(sizeof(double));
    double *b = sf_malloc(sizeof(double));
    double *c = sf_malloc(sizeof(double));
    double *d = sf_malloc(sizeof(double));
    double *e = sf_malloc(sizeof(double));
    double *f = sf_malloc(sizeof(double));
    *a = 5;
    *b = 10;
    *c = 15;
    *d = 20;
    *e = 25;
    *f = 30;
    double *g = sf_realloc(b,25);
    sf_free(g);
    sf_show_heap();
    sf_mem_fini();

    return EXIT_SUCCESS;
}
