#include <stdio.h>
#include <stdlib.h>

#define HEADER_SIZE 44 // Size of WAV file header in bytes

int main(int argc, char *argv[]) {
    FILE *sourceFile, *destFile;
    unsigned char header[HEADER_SIZE]; // To store the WAV file header
    short buffer[2];                   // To store left and right channel samples
    short combined;                    // To store the processed mono sample

    // Ensure the correct number of arguments is provided
    if (argc != 3) {
        fprintf(stderr, "Usage: %s sourcewav destwav\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Open the source WAV file in binary mode
    sourceFile = fopen(argv[1], "rb");
    if (!sourceFile) {
        fprintf(stderr, "Error: Unable to open source file '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Open the destination WAV file in binary mode
    destFile = fopen(argv[2], "wb");
    if (!destFile) {
        fprintf(stderr, "Error: Unable to open destination file '%s'\n", argv[2]);
        fclose(sourceFile); // Ensure the source file is closed
        return EXIT_FAILURE;
    }

    // Copy the header from source to destination
    if (fread(header, sizeof(header), 1, sourceFile) != 1) {
        fprintf(stderr, "Error: Unable to read header from source file\n");
        fclose(sourceFile);
        fclose(destFile);
        return EXIT_FAILURE;
    }

    if (fwrite(header, sizeof(header), 1, destFile) != 1) {
        fprintf(stderr, "Error: Unable to write header to destination file\n");
        fclose(sourceFile);
        fclose(destFile);
        return EXIT_FAILURE;
    }

    // Process the audio data to remove vocals
    while (fread(buffer, sizeof(short), 2, sourceFile) == 2) {
        // Combine the left and right channels into a mono sample
        combined = (buffer[0] - buffer[1]) / 2;
        buffer[0] = buffer[1] = combined;

        // Write the processed mono sample to the destination file
        if (fwrite(buffer, sizeof(short), 2, destFile) != 2) {
            fprintf(stderr, "Error: Unable to write audio data to destination file\n");
            fclose(sourceFile);
            fclose(destFile);
            return EXIT_FAILURE;
        }
    }

    // Close the files
    fclose(sourceFile);
    fclose(destFile);

    return EXIT_SUCCESS;
}
