#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>

#define HEADER_SIZE 44 // Size of WAV file header in bytes

// Function Prototypes
void updateHeaderSizes(FILE *dest, int delay);
void processAudioWithEcho(FILE *source, FILE *dest, int delay, float volume_scale);

int main(int argc, char *argv[]) {
    int delay = 8000;               // Default delay (samples)
    float volume_scale = 4.0;       // Default volume scale factor
    int opt;

    // Parse command-line options using getopt
    while ((opt = getopt(argc, argv, "d:v:")) != -1) {
        switch (opt) {
            case 'd': // Set delay
                delay = atoi(optarg);
                break;
            case 'v': // Set volume scale
                volume_scale = atof(optarg);
                break;
            default: // Invalid option
                fprintf(stderr, "Usage: %s [-d delay] [-v volume_scale] sourcewav destwav\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Validate the number of positional arguments (source and destination files)
    if (argc - optind != 2) {
        fprintf(stderr, "Usage: %s [-d delay] [-v volume_scale] sourcewav destwav\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Assign source and destination file paths
    char *sourcePath = argv[optind];
    char *destPath = argv[optind + 1];

    // Open the source file for reading
    FILE *source = fopen(sourcePath, "rb");
    if (!source) {
        perror("Error opening source file");
        exit(EXIT_FAILURE);
    }

    // Open the destination file for writing
    FILE *dest = fopen(destPath, "wb");
    if (!dest) {
        perror("Error opening destination file");
        fclose(source);
        exit(EXIT_FAILURE);
    }

    // Copy the WAV header from source to destination
    uint8_t header[HEADER_SIZE];
    fread(header, sizeof(uint8_t), HEADER_SIZE, source);
    fwrite(header, sizeof(uint8_t), HEADER_SIZE, dest);

    // Process the audio data to add the echo effect
    processAudioWithEcho(source, dest, delay, volume_scale);

    // Update the WAV header sizes in the destination file
    rewind(dest);
    updateHeaderSizes(dest, delay);

    // Clean up and close files
    fclose(source);
    fclose(dest);

    return 0;
}

// Updates the chunk sizes in the WAV file header to account for added echo
void updateHeaderSizes(FILE *dest, int delay) {
    fseek(dest, 0, SEEK_END);
    long fileSize = ftell(dest); // Get the size of the entire file
    uint32_t chunkSize = fileSize - 8; // Total size minus "RIFF" and chunk size fields
    uint32_t subChunk2Size = fileSize - HEADER_SIZE; // Data size excluding header

    // Update the relevant fields in the header
    fseek(dest, 4, SEEK_SET);
    fwrite(&chunkSize, sizeof(chunkSize), 1, dest);
    fseek(dest, 40, SEEK_SET);
    fwrite(&subChunk2Size, sizeof(subChunk2Size), 1, dest);
}

// Processes the audio data to add echo, writing the modified data to the destination file
void processAudioWithEcho(FILE *source, FILE *dest, int delay, float volume_scale) {
    // Allocate memory for the echo buffer
    short *echoBuffer = (short *)malloc(sizeof(short) * delay);
    if (!echoBuffer) {
        fprintf(stderr, "Failed to allocate echo buffer.\n");
        exit(EXIT_FAILURE);
    }
    memset(echoBuffer, 0, sizeof(short) * delay); // Initialize echo buffer to zero

    int bufferIndex = 0; // Circular buffer index
    short sample;

    // Process each sample from the source file
    while (fread(&sample, sizeof(short), 1, source) == 1) {
        short echoSample = echoBuffer[bufferIndex];
        short scaledEchoSample = echoSample / volume_scale;
        short mixedSample = sample + scaledEchoSample;

        // Clamp mixedSample to the range of int16_t
        if (mixedSample > INT16_MAX) mixedSample = INT16_MAX;
        if (mixedSample < INT16_MIN) mixedSample = INT16_MIN;

        fwrite(&mixedSample, sizeof(short), 1, dest);

        // Update the echo buffer with the current sample
        echoBuffer[bufferIndex] = sample;
        bufferIndex = (bufferIndex + 1) % delay; // Circular buffer logic
    }

    // Write remaining echo samples to the output file
    for (int i = 0; i < delay; ++i) {
        short echoSample = echoBuffer[bufferIndex];
        short scaledEchoSample = echoSample / volume_scale;
        fwrite(&scaledEchoSample, sizeof(short), 1, dest);
        bufferIndex = (bufferIndex + 1) % delay;
    }

    // Free the allocated memory for the echo buffer
    free(echoBuffer);
}
