#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>
#include <mpi.h>

#define SIZE_X 1024
#define SIZE_Y 1024
#define SIZE_Z 64
#define THRESHOLD_LIMIT 25
#define PATTERN_SIZE 4

int cmpfunc (const void * a, const void * b) {
    uint64_t a_value = (uint64_t) *((uint64_t *) a);
    uint64_t b_value = (uint64_t) *((uint64_t *) b);

    if(a_value > b_value)
        return 1;
    else if(a_value < b_value)
        return -1;
    else return 0;
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
            // printf(" - %d occurences of %" PRIu64 " found, adding %d pairs.\n", i_occ, data[i], i_occ * (i_occ - 1));
            count += i_occ * (i_occ  - 1);
        }
    }
    
    return count;
}

void reduce(uint8_t* threshold_array, uint64_t* reduced_array, unsigned int i, unsigned int size_Of_Cluster) {
    // Start sub-timing
    struct timespec start, finish;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);

    const int thread_index = (int)(uintptr_t)i;

    const double min_born = (double)thread_index/size_Of_Cluster;
    const double max_born = (double)(thread_index+1)/size_Of_Cluster;

    const int start_z = min_born*SIZE_Z;
    const int end_z = max_born*SIZE_Z;

    unsigned int reduced_dst_index = 0;

    // Global Iterating
    for(int z = start_z; z <= end_z - PATTERN_SIZE; z += PATTERN_SIZE) {
        for(int y = 0; y <= SIZE_Y - PATTERN_SIZE; y += PATTERN_SIZE) {
            for(int x = 0; x <= SIZE_X - PATTERN_SIZE; x += PATTERN_SIZE) {
                // Reduced cube
                uint64_t threshold = 0;
                unsigned int threshold_bit_index = 0;

                unsigned int index = (z * SIZE_Y * SIZE_X) + (y * SIZE_X) + x;

                // Inner cube iteration
                for(int dz = 0; dz < PATTERN_SIZE; dz++) {
                    for(int dy = 0; dy < PATTERN_SIZE; dy++) {
                        for(int dx = 0; dx < PATTERN_SIZE; dx++) {
                            uint8_t value = threshold_array[((z + dz) * SIZE_Y * SIZE_X) + ((y + dy) * SIZE_X) + x + dx];

                            if(value == 1) {
                                threshold = threshold | 1 << threshold_bit_index;
                            }

                            threshold_bit_index++;
                        }
                    }
                }
                
                reduced_array[reduced_dst_index++] = threshold;
            }
        }
    }

    // Stop sub-timing
    clock_gettime(CLOCK_MONOTONIC, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("  > Time taken to reduce section %d: %f seconds\n", thread_index, elapsed);
}

int main(int argc, char** argv) {
    // Flat 3D array (used for parsing the image by root)
    uint8_t* array;
    // Flat 3D array (used for gathering the thresholded data by everyone)
    uint8_t* threshold_array;
    // 1D array (used for storing reduced thresholded array by everyone)
    uint64_t* reduced_array;
    // 1D array (for gathering the reduced arrays to root)
    uint64_t* gathered_reduced_array;

    // MPI basic init and use of important variable (id and size of communication)
    int process_Rank, size_Of_Cluster;
    const int root_Rank = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);

    // Considere process_Rank 0 as our main
    if (process_Rank == root_Rank) {

        /* Initializing arrays */

        array = (uint8_t*) malloc(SIZE_X * SIZE_Y * SIZE_Z * sizeof(uint8_t));
        if(array == NULL) {
            perror("Error creating array");
            exit(1);
        }

        unsigned int size = (SIZE_X * SIZE_Y * SIZE_Z) / (PATTERN_SIZE * PATTERN_SIZE * PATTERN_SIZE);
        gathered_reduced_array = (uint64_t*) malloc(size * sizeof(uint64_t));
        if(gathered_reduced_array == NULL) {
            perror("Error creating array");
            exit(1);
        }

        printf("* Using MPI with %d process\n", size_Of_Cluster);

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

                    unsigned int index = (z * SIZE_Y * SIZE_X) + (y * SIZE_X) + x;
                    array[index] = voxel;
                }
            }
        }

        fclose(image_raw);

        printf(" - Image imported ! \n");
    }

    // Every process starts here (after main has done initializing image)
    MPI_Barrier(MPI_COMM_WORLD);

    // Start timing
    struct timespec start, finish;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("* [Process %d] Starting computation...\n", process_Rank);

    // Create an array to scater non-thresholded data, equally splitted between process
    unsigned int scatterd_array_size = SIZE_X * SIZE_Y * SIZE_Z / size_Of_Cluster;
    uint8_t* scatterd_array = (uint8_t*) malloc(scatterd_array_size * sizeof(uint8_t));

    // Scatter the array to all our process (everyone works on a part)
    MPI_Scatter(array, scatterd_array_size, MPI_UNSIGNED_CHAR, scatterd_array, scatterd_array_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    printf("* [Process %d] Thresholding the voxels...\n", process_Rank);

    // Thresholding voxels (every process works on its scattered data)
    for (unsigned int i = 0; i < scatterd_array_size; i++) {
        uint8_t voxel = scatterd_array[i];
        scatterd_array[i] = (voxel > THRESHOLD_LIMIT) ? 1 : 0;
    }
    
    printf("* [Process %d] Thresholding of voxels finished !\n", process_Rank);

    // Allgather -> everybody receive the same array of thresholded data
    /*
     * +-----------+  +-----------+  +-----------+
     * | Process 0 |  | Process 1 |  | Process 2 |
     * +-+-------+-+  +-+-------+-+  +-+-------+-+
     *   | Value |      | Value |      | Value |
     *   |   0   |      |  100  |      |  200  |
     *   +-------+      +-------+      +-------+
     *       |________      |      ________|
     *                |     |     | 
     *             +-----+-----+-----+
     *             |  0  | 100 | 200 |
     *             +-----+-----+-----+
     *             |   Each process  |
     *             +-----------------+
     *
     */

    threshold_array = (uint8_t*) malloc(SIZE_X * SIZE_Y * SIZE_Z * sizeof(uint8_t));
    if(threshold_array == NULL) {
        perror("Error creating array");
        exit(1);
    }

    MPI_Allgather(scatterd_array, scatterd_array_size, MPI_UNSIGNED_CHAR, threshold_array, scatterd_array_size, MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    // Everyone reduce their threshold_array to a reduced array
    // Create an array to collect reduced data
    unsigned int size = (SIZE_X * SIZE_Y * SIZE_Z) / (PATTERN_SIZE * PATTERN_SIZE * PATTERN_SIZE); 
    unsigned int reduced_array_size = size / size_Of_Cluster;
    reduced_array = (uint64_t*) malloc(reduced_array_size * sizeof(uint64_t));

    // Now reducing the array to a uint64_t array

    printf("* [Process %d] Reducing the array...\n", process_Rank);

    reduce(threshold_array, reduced_array, process_Rank, size_Of_Cluster);
    
    printf("* [Process %d] Finished reducing the array !\n", process_Rank);

    // Now gather to root process to sort and count pairs
    // Gather -> Only root received the data (not everyone as in Allgather)
    if (process_Rank == root_Rank) {
        MPI_Gather(reduced_array, reduced_array_size, MPI_UNSIGNED_LONG_LONG, gathered_reduced_array, reduced_array_size, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    } else {
        MPI_Gather(reduced_array, reduced_array_size, MPI_UNSIGNED_LONG_LONG, NULL, 0, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Stop here if not main
    if(process_Rank == root_Rank) {
        // Same computations as pairs_sequential from now on

        printf("* [Process %d] Sorting the array...\n", process_Rank);

        qsort(gathered_reduced_array, size, sizeof(uint64_t), cmpfunc);

        printf("* [Process %d] Finished sorting the array !\n", process_Rank);

        printf("* [Process %d] Counting pairs...\n", process_Rank);

        unsigned int numPairs = countPairs(gathered_reduced_array, size);

        printf("* [Process %d] Finished counting pairs !\n", process_Rank);

        // Stop timing
        clock_gettime(CLOCK_MONOTONIC, &finish);
        elapsed = (finish.tv_sec - start.tv_sec);
        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

        printf(" - Number of pattern-copy pairs found: %d\n", numPairs);

        printf("> Time taken: %f seconds\n", elapsed);

        free(array);
        free(gathered_reduced_array);
    }

    free(reduced_array);
    free(threshold_array);

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();

    return 0;
}