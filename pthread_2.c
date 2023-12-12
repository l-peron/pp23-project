#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>

#define SIZE_X 1024
#define SIZE_Y 1024
#define SIZE_Z 64
#define THRESHOLD_LIMIT 25
#define PATTERN_SIZE 4
#define MAX_THREAD 16

pthread_t threads[MAX_THREAD];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
uint8_t*** threshold_array;
uint64_t* reduced_array;
uint8_t*** array;

int cmpfunc (const void * a, const void * b) {
    uint64_t a_value = (uint64_t) *((uint64_t *) a);
    uint64_t b_value = (uint64_t) *((uint64_t *) b);

    if(a_value > b_value)
        return 1;
    else if(a_value < b_value)
        return -1;
    else return 0;
}

/* 
 * reduce(uint8_t*** data, uint64_t* reduced_dst) -> void :
 *  Iterate through the array, parsing (4 * 4 * 4) cubes of value of only 0 and 1
 *  And reducing them to 64 bits numbers
 */
void reduce(uint8_t*** data, uint64_t* reduced_dst) {
    unsigned int reduced_dst_index = 0;

    // Global Iterating
    for(int z = 0; z <= SIZE_Z - PATTERN_SIZE; z += PATTERN_SIZE) {
        for(int y = 0; y <= SIZE_Y - PATTERN_SIZE; y += PATTERN_SIZE) {
            for(int x = 0; x <= SIZE_X - PATTERN_SIZE; x += PATTERN_SIZE) {
                // Reduced cube
                uint64_t threshold = 0;
                unsigned int threshold_bit_index = 0;

                // Inner cube iteration
                for(int dz = 0; dz < PATTERN_SIZE; dz++) {
                    for(int dy = 0; dy < PATTERN_SIZE; dy++) {
                        for(int dx = 0; dx < PATTERN_SIZE; dx++) {
                            uint8_t value = data[x + dx][y + dy][z + dz];

                            if(value == 1) {
                                threshold = threshold | 1 << threshold_bit_index;
                            }

                            threshold_bit_index++;
                        }
                    }
                }

                reduced_dst[reduced_dst_index++] = threshold;
            }
        }
    }
}

unsigned int countPairs(uint64_t* data, unsigned int size) {
    unsigned int count = 0;

    // Create a visited array to note the one we don't need to compute again
    char* visited = malloc(sizeof(char) * size);
    if(visited == NULL) {
        perror("Couldn't create the array");
        exit(0);
    }
    // Set the whole visited to 0
    memset(visited, 0, size);

    // The data is mostly filled with zeros
    // We skip them, and then we count only the zeros
    for(unsigned int i = 0; i < size; i++) {
        // Skip the already counted
        if(visited[i] == 1) continue;

        unsigned int i_occ = 1;

        // Since the array is sorted, if we find a higher value, we won't lower down again
        char lower = 1;

        // Count the pairs
        for(unsigned int j = i + 1; j < size && lower; j++) {
            // Lower check
            if(data[i] < data[j]) {
                lower = 0;
                break;
            }
            // Visited check
            if(data[i] == data[j]) {
                visited[j] = 1;
                i_occ++;
            }
        }

        // If we found a pair
        // Since we are not visiting the other pairs (marked as visited), we need to add their values too
        // Every pair would have found the (i_occ - 1) other, so add i_occ * (i_occ - 1)
        if(i_occ > 1) {
            //printf(" - %d occurences of %" PRIu64 " found, adding %d pairs.\n", i_occ, data[i], i_occ * (i_occ - 1));
            count += i_occ * (i_occ  - 1);
        }
    }
    
    return count;
}

void *thread_thresholding(void *i) {
    // Start sub-timing
    clock_t start_time = clock();

    const int thread_index = (int)i;

    const double min_born = (double)thread_index/MAX_THREAD;
    const double max_born = (double)(thread_index+1)/MAX_THREAD;

    const int start_z = min_born*SIZE_Z;
    const int end_z = max_born*SIZE_Z;

	printf("* Thresholding in section: %d\n", i);
    printf("* Z Range: [%d, %d[\n", start_z, end_z);

    int count = 0;

    // Thresholding voxels
    for (int z = start_z; z < end_z; z++) {
        for (int y = 0; y < SIZE_Y; y++) {
            for (int x = 0; x < SIZE_X; x++) {
                uint8_t voxel = array[x][y][z];
                threshold_array[x][y][z] = (voxel > THRESHOLD_LIMIT) ? 1 : 0;
                if(voxel > THRESHOLD_LIMIT) count++;
            }
        }
    }

    printf("count: %d", count);

    // Stop sub-timing
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf(" - Time taken to threshold section %d: %f seconds\n", i, elapsed_time);
	pthread_exit(NULL);
}

int main() {
    threshold_array = (uint8_t***) malloc(SIZE_X * sizeof(uint8_t**));

    if(threshold_array == NULL) {
        perror("Error creating array");
        exit(1);
    }

	for (int i = 0; i < SIZE_X; i++) {
		threshold_array[i] = (uint8_t**) malloc(SIZE_Y * sizeof(uint8_t*));
        if(threshold_array[i] == NULL) {
            perror("Error creating array");
            exit(1);
        }
	}
	for (int i = 0; i < SIZE_X; ++i) { 
		for (int j = 0; j < SIZE_Y; ++j) {
			threshold_array[i][j] = (uint8_t*) malloc(SIZE_Z * sizeof(uint8_t));
            if(threshold_array[i][j] == NULL) {
                perror("Error creating array");
                exit(1);
            }
		}
	}

    /* Initializing Image array */

    array = (uint8_t***) malloc(SIZE_X * sizeof(uint8_t**));

    if(array == NULL) {
        perror("Error creating array");
        exit(1);
    }

	for (int i = 0; i < SIZE_X; i++) {
		array[i] = (uint8_t**) malloc(SIZE_Y * sizeof(uint8_t*));
        if(array[i] == NULL) {
            perror("Error creating array");
            exit(1);
        }
	}
	for (int i = 0; i < SIZE_X; ++i) { 
		for (int j = 0; j < SIZE_Y; ++j) {
			array[i][j] = (uint8_t*) malloc(SIZE_Z * sizeof(uint8_t));
            if(array[i][j] == NULL) {
                perror("Error creating array");
                exit(1);
            }
		}
	}

    FILE *image_raw = fopen("./c8.raw", "rb");

    if (image_raw == NULL) {
        perror("Error opening file");
        exit(1);
    }

    // Importing the image
    printf("* Importing the image... \n");

    for (int z = 0; z < SIZE_Z; ++z) {
        for (int y = 0; y < SIZE_Y; ++y) {
            for (int x = 0; x < SIZE_X; ++x) {
                uint8_t voxel;
                fread(&voxel, sizeof(uint8_t), 1, image_raw);
                array[x][y][z] = voxel;
            }
        }
    }

    fclose(image_raw);

    printf(" - Image imported ! \n");

    printf("* Starting computation...\n");

    // Start timing
    clock_t start_time = clock();

    printf("* Thresholding the voxels...\n");

    for(int i=0; i< MAX_THREAD; i++) {
        pthread_create(&threads[i], NULL, thread_thresholding, (void *)i);
        pthread_join(threads[i], NULL);
    }

    printf("* Thresholding of voxels finished !\n");

    // Now reducing the array to a uint64_t array

    printf("* Reducing the array...\n");

    unsigned int size = (SIZE_X * SIZE_Y * SIZE_Z) / (PATTERN_SIZE * PATTERN_SIZE * PATTERN_SIZE);
    reduced_array = malloc(sizeof(uint64_t) * size);
    if(reduced_array == NULL) {
        perror("Error creating array");
    }

    reduce(threshold_array, reduced_array);

    printf("* Finished reducing the array !\n");

    printf(" - Reduced array length is : %d\n", size);

    printf("* Sorting the array...\n");

    qsort(reduced_array, size, sizeof(uint64_t), cmpfunc);

    printf("* Finished sorting the array !\n");

    printf("* Counting pairs...\n");

    unsigned int numPairs = countPairs(reduced_array, size);

    printf("* Finished counting pairs !\n");

    // Stop timing
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf(" - Number of pattern-copy pairs found: %d\n", numPairs);

    printf("> Time taken: %f seconds\n", elapsed_time);

    return 0;
}