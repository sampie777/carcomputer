//
// Created by samuel on 2025/02/15.
//

#include "math.h"

double sign(double x) {
    if (x == 0) return 1;
    return x / sqrt(x * x);
}

double abs_double(double x) {
    return sqrt(x * x);
}

double vector2_length(double x, double y) {
    return sqrt(x * x + y * y);
}

double vector3_length(double x, double y, double z) {
    return sqrt(x * x + y * y + z * z);
}

void cartesian_to_spherical(double x, double y, double z, double *r, double *theta, double *phi) {
    *r = vector3_length(x, y, z);
    if (*r == 0) {
        *theta = 0;
        *phi = 0;
        return;
    }

    *theta = acos(z / *r);
    *phi = atan2(y, x);

    if (*phi == -1 * M_PI) *phi = M_PI;
}

void spherical_to_cartesian(double r, double theta, double phi, double *x, double *y, double *z) {
    if (r == 0) {
        *x = 0;
        *y = 0;
        *z = 0;
        return;
    }
    *x = r * sin(theta) * cos(phi);
    *y = r * sin(theta) * sin(phi);
    *z = r * cos(theta);
}

void cartesian_to_spherical_vectors(Vector3 input, Vector3Spherical *output) {
    cartesian_to_spherical(input.x, input.y, input.z, &output->r, &output->theta, &output->phi);
}

void spherical_to_cartesian_vectors(Vector3Spherical input, Vector3 *output) {
    spherical_to_cartesian(input.r, input.theta, input.phi, &output->x, &output->y, &output->z);
}

double rad_to_deg(double rad) {
    return rad * 180 / M_PI;
}

double deg_to_rad(double deg) {
    return deg * M_PI / 180;
}

void normalize(Vector3 *v) {
    double mag = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    if (mag > 0.0001) {  // Avoid division by zero
        v->x /= mag;
        v->y /= mag;
        v->z /= mag;
    }
}

void compute_rotation_matrix(double rotation_matrix[3][3], Vector3 initial) {
    normalize(&initial);  // Ensure unit vector

    // Assume we want to rotate this initial reading to (0, 0, 1)
    Vector3 target = {0, 0, -1};

    // Compute cross product to get the rotation axis
    Vector3 axis = {
        initial.y * target.z - initial.z * target.y,
        initial.z * target.x - initial.x * target.z,
        initial.x * target.y - initial.y * target.x
    };

    double dot = initial.x * target.x + initial.y * target.y + initial.z * target.z;
    double angle = acos(dot);  // Angle between the two vectors

    // Normalize the axis
    normalize(&axis);

    // Compute rotation matrix using Rodrigues' formula
    double c = cos(angle);
    double s = sin(angle);
    double t = 1 - c;

    rotation_matrix[0][0] = t * axis.x * axis.x + c;
    rotation_matrix[0][1] = t * axis.x * axis.y - s * axis.z;
    rotation_matrix[0][2] = t * axis.x * axis.z + s * axis.y;

    rotation_matrix[1][0] = t * axis.x * axis.y + s * axis.z;
    rotation_matrix[1][1] = t * axis.y * axis.y + c;
    rotation_matrix[1][2] = t * axis.y * axis.z - s * axis.x;

    rotation_matrix[2][0] = t * axis.x * axis.z - s * axis.y;
    rotation_matrix[2][1] = t * axis.y * axis.z + s * axis.x;
    rotation_matrix[2][2] = t * axis.z * axis.z + c;
}

void rotate_accelerometer(double rotation_matrix[3][3], Vector3 *input, Vector3 *output) {
    output->x = rotation_matrix[0][0] * input->x +
        rotation_matrix[0][1] * input->y +
        rotation_matrix[0][2] * input->z;

    output->y = rotation_matrix[1][0] * input->x +
        rotation_matrix[1][1] * input->y +
        rotation_matrix[1][2] * input->z;

    output->z = rotation_matrix[2][0] * input->x +
        rotation_matrix[2][1] * input->y +
        rotation_matrix[2][2] * input->z;
}

void rotate_vector(double rotation_matrix[3][3], Vector3 *input) {
    Vector3 rotated_reading;
    rotate_accelerometer(rotation_matrix, input, &rotated_reading);
    input->x = rotated_reading.x;
    input->y = rotated_reading.y;
    input->z = rotated_reading.z;
}
