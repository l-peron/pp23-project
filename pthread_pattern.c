#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>


#define SIZE_X 1024
#define SIZE_Y 1024
#define SIZE_Z 64
#define THRESHOLD_LIMIT 25
#define PATTERN_SIZE 4
#define MAX_THREAD 256

int num_pairs = 0;
int*** threshold_array;
pthread_t threads[MAX_THREAD];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int findPatternCopyPairs(int*** data, int i) {
    int numPairs = 0;

    int init_z = SIZE_Z*(int)i/MAX_THREAD;
    int max_z = SIZE_Z*(int)(i+1)/MAX_THREAD > SIZE_Z - PATTERN_SIZE ? SIZE_Z - PATTERN_SIZE : SIZE_Z*(int)(i+1)/MAX_THREAD;
    
    int init_y = SIZE_Y*(int)i/MAX_THREAD;
    int max_y = SIZE_Y*(int)(i+1)/MAX_THREAD > SIZE_Y - PATTERN_SIZE ? SIZE_Y - PATTERN_SIZE : SIZE_Y*(int)(i+1)/MAX_THREAD;

    int init_x = SIZE_X*(int)i/MAX_THREAD;
    int max_x = SIZE_X*(int)(i+1)/MAX_THREAD > SIZE_X - PATTERN_SIZE ? SIZE_X - PATTERN_SIZE : SIZE_X*(int)(i+1)/MAX_THREAD;

    // Global Iterating
    for(int z = init_z; z <= max_z; z += PATTERN_SIZE) {
        for(int y = init_y; y <= max_y; y += PATTERN_SIZE) {
            for(int x = init_x; x <= max_x; x += PATTERN_SIZE) {

                // Pattern Iterating
                for(int p_z = z + PATTERN_SIZE; p_z <= SIZE_Z - PATTERN_SIZE; p_z += PATTERN_SIZE) {
                    for(int p_y = y + PATTERN_SIZE; p_y <= SIZE_Y - PATTERN_SIZE; p_y += PATTERN_SIZE) {
                        for(int p_x = x + PATTERN_SIZE; p_x <= SIZE_X - PATTERN_SIZE; p_x += PATTERN_SIZE) {

                            // Pattern Compare
                            int match = 1;
                            for(int dz = 0; dz < PATTERN_SIZE && match; ++dz) {
                                for(int dy = 0; dy < PATTERN_SIZE && match; ++dy) {
                                    for(int dx = 0; dx < PATTERN_SIZE && match; ++dx) {
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

void *thread_searching(void *i) {
	printf("Searching in sub-section: %d, ", i);

    // Start sub-timing
    clock_t start_time = clock();

    int local_pairs = findPatternCopyPairs(threshold_array, i);

    // Stop sub-timing
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("pairs found: %d \n", local_pairs);
    printf("Time taken: %f seconds\n", elapsed_time);

    pthread_mutex_lock(&mutex);
    num_pairs += local_pairs;
    pthread_mutex_unlock(&mutex);

	pthread_exit(NULL);
}

int main(int argc, char* argv[]) {

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

    for(int i=0; i< MAX_THREAD; i++) {
        pthread_create(&threads[i], NULL, thread_searching, (void *)i);
        pthread_join(threads[i], NULL);
    }

    // Stop timing
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("Number of pattern-copy pairs found: %d\n", num_pairs);
    printf("Time taken: %f seconds\n", elapsed_time);

    return 0;
    
}
