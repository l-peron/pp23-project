## Parallel programming - Assignment II No.1

In C programming language, implement a calculation that:

1. Loads a CT image with a resolution of 1024 x 1024 x 314 voxels from the file c8.raw, while only the first 64 slices are loaded from it (so 1024 x 1024 x 64). Each voxel is recorded on 8 bits as uint8. Voxel values ​​range from <0;255>. The voxels are arranged in the file to represent a dump of the v[x][y][z] field. The X coordinate is incremented first, then Y and finally Z.
1. Apply thresholding to each of the voxels so that the threshold value is 25. This will result in metadata with 1b/voxel, where this metadata value will be
   1. equal to 0 for a voxel whose value is less than or equal to the threshold value and the voxel will be considered passive,
   1. equal to 1 for a voxel whose value is greater than the threshold value and the voxel will be considered active.
1. Search within the CT for all pattern (A) - copy (B) pairs, where both the pattern and the copy have dimensions of 4 x 4 x 4 voxels. The pattern moves by 4 pixels each time. A pattern matches its copy when, for each of its 64 voxels, it is true that if the respective voxel is active in the pattern, it is also active in the copy, and conversely, if it is passive in the pattern, it is also passive in the copy. If A was a pattern and B was a copy, then the situation where B is a pattern and A is a copy will be taken into account as the occurrence of another pattern-copy pair.
1. Count how many pattern-copy pairs are found in the read data.

1. Perform the calculation sequentially and measure the time required to perform the calculation. Start the measurement after loading the data from the disk.
1. Perform the calculation in parallel, using the pthread.h library. Choose a different number of threads, decompose the problem and implement a parallel computation using the appropriate parallelism. Measure the time required to perform the calculation for different number of threads. Always start the measurement after loading the data from the disk.
1. Perform the calculation as parallel, using the MPI library. Choose a different number of processes, decompose the problem and, using the appropriate parallelism, implement a parallel calculation. Measure the time required to perform the calculation for different number of processes. Always start the measurement after loading the data from the disk.

In the documentation, state the task, outline the solution method, and in the form of a table and graph, state the calculation execution time for sequential as well as parallel execution with different numbers of threads or processes. Comment on the measured times. Comment on the time complexity of the used problems or algorithms. Please provide the source code of the program.


