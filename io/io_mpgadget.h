#ifndef _IO_MPGADGET_H
#define _IO_MPGADGET_H

#include <stdint.h>
#include "../particle.h"

struct Point3D{double x,y,z;};

void load_particles_mpgadget(char *filename, struct particle **p, int64_t *num_p);

#endif /*_IO_MPGADGET_H */