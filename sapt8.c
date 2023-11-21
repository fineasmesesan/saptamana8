#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <dirent.h>
#include <time.h>

#define BUFFER_SIZE 1024

typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t important_colors;
} BitmapHeader;

int calculatePixelDataOffset(BitmapHeader *header) {
    return header->offset;
}

int calculatePixelDataSize(BitmapHeader *header) {
    int dataSize = (header->width * header->height) * (header->bits_per_pixel / 8);
    return dataSize;
}

void processImage(const char *imageName, int pixelDataOffset, int pixelDataSize) {
    int imageFile = open(imageName, O_RDWR);
    if (imageFile == -1) {
        perror("Eroare la deschiderea fis BMP");
        exit(1);
    }
    //se pune cursorul la inceput
    lseek(imageFile, pixelDataOffset, SEEK_SET);
    // Se parcurge fiecare pixel
    unsigned char pixel[3];  //initializare pentru R, G, B
    while (read(imageFile, pixel, 3) == 3) {
      unsigned char grayPixel = (unsigned char)(0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]); //formula
        
        pixel[0] = pixel[1] = pixel[2] = grayPixel;
	//se muta cursorul inapoi la pozitia initiala 
        lseek(imageFile, -3, SEEK_CUR);
        write(imageFile, pixel, 3);
    }

    close(imageFile);
}

void writeStatistics(const char *elementName, struct stat *elementInfo, const char *outputDirectory) {
    char outputFile[BUFFER_SIZE];
    snprintf(outputFile, sizeof(outputFile), "%s/%s_statistics.txt", outputDirectory, elementName);

    int statsFile = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (statsFile == -1) {
        perror("eroare scriere/citire");
        return;
    }

    char statsInfo[BUFFER_SIZE];
    sprintf(statsInfo, "nume fisier: %s\nSize: %ld bytes\nLast Modified: %s"
                        "\nPermissions: %o\n----\n",
            elementName, elementInfo->st_size, ctime(&elementInfo->st_mtime), elementInfo->st_mode);

    if (write(statsFile, statsInfo, strlen(statsInfo)) == -1) {
        perror("eroare la scriere");
    }

    close(statsFile);
}

void processElement(const char *elementName, const char *outputDirectory) {
    char elementPath[BUFFER_SIZE];
    snprintf(elementPath, sizeof(elementPath), "%s/%s", outputDirectory, elementName);

    struct stat elementInfo;
    if (lstat(elementPath, &elementInfo) == -1) {
        perror("Eroare");
        return;
    }

    if (S_ISREG(elementInfo.st_mode)) {
        writeStatistics(elementName, &elementInfo, outputDirectory);

        if (strstr(elementName, ".bmp") != NULL) {
            int pixelDataOffset = calculatePixelDataOffset((BitmapHeader *)&elementInfo);
            int pixelDataSize = calculatePixelDataSize((BitmapHeader *)&elementInfo);

            pid_t pid = fork();
            if (pid < 0) {
                perror("eroare la forking");
                return;
            } else if (pid == 0) {
                processImage(elementName, pixelDataOffset, pixelDataSize);
                exit(0);
            }
        }
    }
 }

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "foloseste: %s <input_directory> <output_directory>\n", argv[0]);
        return 1;
    }

    processElement(argv[1], argv[2]);

    return 0;
}

