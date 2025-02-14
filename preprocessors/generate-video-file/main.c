#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <math.h>

#define FRAME_COUNT (600)
#define HEIGHT (64)
#define WIDTH (128)
#define ROW_COUNT (HEIGHT >> 3)
#define COLUMN_COUNT WIDTH

int save_output(char ***matrix) {
    if (matrix == NULL) return -1;

    FILE *file = fopen("matrix_output.bin", "wb");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }

    for (int i = 0; i < FRAME_COUNT; i++) {
        for (int row = 0; row < ROW_COUNT; row++) {
            fwrite(matrix[i][row], sizeof(char), COLUMN_COUNT, file);
        }
    }
    fclose(file);
    return 0;
}

void convert_to_output_matrix(char ***input, char ***output) {
    if (input == NULL) return;
    if (output == NULL) return;

    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        for (int col = 0; col < WIDTH; col++) {
            for (int row = 0; row < ROW_COUNT; row++) {
                char byte = 0;
                for (int bit = 0; bit < 8; bit++) {
                    if (input[frame][row * 8 + bit][col]) {
                        byte |= (1 << bit);
                    }
                }
                output[frame][row][col] = byte;
            }
        }
    }
}

void media_to_matrix(char ***matrix) {
    if (matrix == NULL) return;

    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        int radius = 8 + sin((double) frame / FRAME_COUNT * M_PI) * 5;
        int centerX = WIDTH / 2;
        int centerY = HEIGHT / 2;
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int dx = x - centerX;
                int dy = y - centerY;
                if (dx * dx + dy * dy <= radius * radius && dx * dx + dy * dy >= (radius - 1) * (radius - 1)) {
                    matrix[frame][y][x] = 1;
                }
            }
        }
    }

    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        int radius = 10 + sin((double) frame / FRAME_COUNT * 2 * M_PI) * 5;
        int centerX = 35;
        int centerY = 12;
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int dx = x - centerX;
                int dy = y - centerY;
                if (dx * dx + dy * dy <= radius * radius && dx * dx + dy * dy >= (radius - 1) * (radius - 1)) {
                    matrix[frame][y][x] = 1;
                }
            }
        }
    }

    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        int radius = 30 + sin((double) frame / FRAME_COUNT * 0.2 * M_PI) * 15;
        int centerX = 100;
        int centerY = 50;
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int dx = x - centerX;
                int dy = y - centerY;
                if (dx * dx + dy * dy <= radius * radius && dx * dx + dy * dy >= (radius - 1) * (radius - 1)) {
                    matrix[frame][y][x] = 1;
                }
            }
        }
    }
}

char ***create_matrix(int frame_count, int height, int width) {
    char ***from_matrix = NULL;
    from_matrix = malloc(frame_count * sizeof(char **));
    for (int i = 0; i < frame_count; i++) {
        from_matrix[i] = malloc(height * sizeof(char *));
        for (int j = 0; j < height; j++) {
            from_matrix[i][j] = malloc(width * sizeof(char));
        }
    }
    return from_matrix;
}

void free_matrix(char ***matrix, int frame_count, int height) {
    if (matrix == NULL) return;

    for (int i = 0; i < frame_count; i++) {
        for (int j = 0; j < height; j++) {
            free(matrix[i][j]);
        }
        free(matrix[i]);
    }
    free(matrix);
}

void display(char ***matrix) {
    if (matrix == NULL) return;

    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        for (int row = 0; row < ROW_COUNT - 3; row++) {
            for (int bit = 0; bit < 8; bit++) {
                for (int col = 0; col < COLUMN_COUNT; col++) {
                    printf("%c", matrix[frame][row][col] & (1 << bit) ? '#' : '.');
                }
                printf("\n");
            }
        }
        printf("\n\n");
        usleep(1000 * 235);
    }
}

int main() {
    char ***from_matrix = create_matrix(FRAME_COUNT, HEIGHT, WIDTH);
    char ***output_matrix = create_matrix(FRAME_COUNT, ROW_COUNT, COLUMN_COUNT);

    media_to_matrix(from_matrix);
    convert_to_output_matrix(from_matrix, output_matrix);
    save_output(output_matrix);
//    display(output_matrix);


    free_matrix(from_matrix, FRAME_COUNT, HEIGHT);
    free_matrix(output_matrix, FRAME_COUNT, ROW_COUNT);
    return 0;
}
