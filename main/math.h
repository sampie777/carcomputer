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

void cartesian_to_spherical(double x, double y, double z, double *r, double *theta, double *phi);
void cartesian_to_spherical_vectors(Vector3 input, Vector3Spherical *output);

void spherical_to_cartesian(double r, double theta, double phi, double *x, double *y, double *z);
void spherical_to_cartesian_vectors(Vector3Spherical input, Vector3 *output);

void compute_rotation_matrix(double rotation_matrix[3][3], Vector3 initial);
void rotate_vector(double rotation_matrix[3][3], Vector3 *input);
void bound_spherical(Vector3Spherical *vector);

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

#endif //CARCOMPUTER_MATH_H
