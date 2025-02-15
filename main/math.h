//
// Created by samuel on 2025/02/15.
//

#ifndef CARCOMPUTER_MATH_H
#define CARCOMPUTER_MATH_H

#include <math.h>

typedef struct {
    double x;
    double y;
    double z;
} Vector3;

typedef struct {
    double r;       // [0, ->]
    double theta;   // [0, M_PI]
    double phi;     // (-M_PI, M_PI]
} Vector3Spherical;

double rad_to_deg(double rad);
double deg_to_rad(double deg);

void bound_spherical(Vector3Spherical *vector);
void rotate_spherical(Vector3Spherical *vector, double theta, double phi);
// 2x or 3x times faster than rotate_spherical, but more difficult to maintain
void rotate_spherical_fast(Vector3Spherical *vector, double theta, double phi);

void cartesian_to_spherical(double x, double y, double z, double *r, double *theta, double *phi);
void cartesian_to_spherical_vectors(Vector3 input, Vector3Spherical *output);

void spherical_to_cartesian(double r, double theta, double phi, double *x, double *y, double *z);
void spherical_to_cartesian_vectors(Vector3Spherical input, Vector3 *output);

#endif //CARCOMPUTER_MATH_H
