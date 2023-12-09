#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define SIZE_X 1024/2
#define SIZE_Y 1024/2
#define SIZE_Z 64/2
#define THRESHOLD_LIMIT 25
#define PATTERN_SIZE 4

int findPatternCopyPairs(int*** data) {
    int numPairs = 0;

    // Global Iterating
    for(int z = 0; z <= SIZE_Z - PATTERN_SIZE; z += PATTERN_SIZE) {
        for(int y = 0; y <= SIZE_Y - PATTERN_SIZE; y += PATTERN_SIZE) {
            for(int x = 0; x <= SIZE_X - PATTERN_SIZE; x += PATTERN_SIZE) {

                // Pattern Iterating
                for(int p_z = z + PATTERN_SIZE; p_z <= SIZE_Z - PATTERN_SIZE; p_z += PATTERN_SIZE) {
                    for(int p_y = y + PATTERN_SIZE; p_y <= SIZE_Y - PATTERN_SIZE; p_y += PATTERN_SIZE) {
                        for(int p_x = x + PATTERN_SIZE; p_x <= SIZE_X - PATTERN_SIZE; p_x += PATTERN_SIZE) {

                            // Pattern Compare
                            int match = 1;
                            for(int dz = 0; dz < PATTERN_SIZE && match; dz++) {
                                for(int dy = 0; dy < PATTERN_SIZE && match; dy++) {
                                    for(int dx = 0; dx < PATTERN_SIZE && match; dx++) {
                                        if(data[x + dx][y + dy][z + dz] != data[p_x + dx][p_y + dy][p_z + dz]) {
                                            match = 0;
                                        }
                                    }
                                }
                            }
                            if(match) numPairs++;
                        }
                    }
                }
            }
        }
    }

    return numPairs;
}

int main() {

    int*** threshold_array;
    threshold_array = (int***)malloc(SIZE_X * sizeof(int**));
	for (int i = 0; i < SIZE_X; i++) {
		threshold_array[i] = (int**)malloc(SIZE_Y * sizeof(int*));
	}
	for (int i = 0; i < SIZE_X; ++i) { 
		for (int j = 0; j < SIZE_Y; ++j) {
			threshold_array[i][j] = (int*)malloc(SIZE_Z * sizeof(int));
		}
	}

    FILE *image_raw = fopen("./c8.raw", "rb");

    if (image_raw == NULL) {
        perror("Error opening file");
        exit(1);
    }

    fclose(image_raw);

    // Start timing
    clock_t start_time = clock();

    // Thresholding voxels
    for (int z = 0; z < SIZE_Z; ++z) {
        for (int y = 0; y < SIZE_Y; ++y) {
            for (int x = 0; x < SIZE_X; ++x) {
                uint8_t voxel;
                fread(&voxel, sizeof(uint8_t), 1, image_raw);
                threshold_array[x][y][z] = (voxel > THRESHOLD_LIMIT) ? 1 : 0;
            }
        }
    }

    // Find pattern-copy pairs
    int numPairs = findPatternCopyPairs(threshold_array);

    // Stop timing
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("Number of pattern-copy pairs found: %d\n", numPairs);
    printf("Time taken: %f seconds\n", elapsed_time);

    return 0;
}
