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

void bound_spherical(Vector3Spherical *vector) {
    vector->theta = fmod(vector->theta, M_PI);
    // Check if theta should have been 180 degrees
    if (vector->theta == 0 && fmod(vector->theta, 2 * M_PI) < M_PI) {
        vector->theta = M_PI;
    }

    vector->phi = fmod(vector->phi, 2 * M_PI);
    if (vector->phi < M_PI) {
        vector->phi = fmod(vector->phi + 2 * M_PI, 2 * M_PI);
    }
    if (vector->phi > M_PI) {
        vector->phi = fmod(vector->phi - 2 * M_PI, 2 * M_PI);
    }
}

void rotate_spherical(Vector3Spherical *vector, double theta, double phi) {
    Vector3Spherical temp = {
        .r = vector->r,
        .theta = vector->theta,
        .phi = vector->phi,
    };

    temp.theta += theta;

    Vector3 temp_cartesian = {0};
    spherical_to_cartesian_vectors(temp, &temp_cartesian);
    cartesian_to_spherical_vectors(temp_cartesian, &temp);
    vector->theta = temp.theta;

    // No need to calculate phi if the vector is vertical
    if (temp.theta == 0 || temp.theta == M_PI) return;

    temp.phi += phi;
    spherical_to_cartesian_vectors(temp, &temp_cartesian);
    cartesian_to_spherical_vectors(temp_cartesian, &temp);

    vector->theta = temp.theta;
    vector->phi = temp.phi;
}

void rotate_spherical_fast(Vector3Spherical *vector, double theta, double phi) {
    if (theta == 0 && phi == 0) return;

    if (theta != 0) {
        double new_theta = M_PI - vector->theta + theta;
        vector->theta = fmod(new_theta, 2 * M_PI);

        if (fabs(vector->theta) > M_PI) {
            vector->theta -= sign(vector->theta) * 2 * M_PI;
        }
        if (vector->theta == -1 * M_PI) {
            vector->theta *= -1;
        }

        if (vector->theta < 0) {
            vector->theta *= -1;
            vector->phi = fmod(vector->phi + M_PI, 2 * M_PI);
        }
    }

    // No need to calculate phi if the vector is vertical
    if (vector->theta == 0 || vector->theta == M_PI || phi == 0) return;

    vector->phi += phi;
    vector->phi = fmod(vector->phi, 2 * M_PI);

    if (fabs(vector->phi) > M_PI) {
        vector->phi -= sign(vector->phi) * 2 * M_PI;
        vector->phi = fmod(vector->phi, 2 * M_PI);
    }
    if (vector->phi == -1 * M_PI) {
        vector->phi *= -1;
    }
}
