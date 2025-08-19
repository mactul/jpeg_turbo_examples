#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <turbojpeg.h>

// Helper function: load entire file into memory
unsigned char* load_file(const char* filename, unsigned long* size)
{
    FILE* file = fopen(filename, "rb");
    if(!file)
    {
        perror("fopen");
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char* buf = malloc(*size);
    if(!buf)
    {
        perror("malloc");
        fclose(file);
        return NULL;
    }
    fread(buf, 1, *size, file);
    fclose(file);
    return buf;
}

// Save raw RGB buffer as JPEG
int save_jpeg(const char* filename, unsigned char* buffer, int width, int height, int quality)
{
    tjhandle handle        = NULL;
    unsigned char* jpegBuf = NULL;
    unsigned long jpegSize = 0;

    FILE* outfile = fopen(filename, "wb");
    if(!outfile)
    {
        perror("fopen");
        return -1;
    }

    handle = tjInitCompress();
    if(!handle)
    {
        fprintf(stderr, "tjInitCompress error: %s\n", tjGetErrorStr());
        fclose(outfile);
        return -1;
    }

    // Compress raw RGB into JPEG buffer
    if(tjCompress2(
           handle,
           buffer,
           width,
           0,
           height,
           TJPF_RGB,
           &jpegBuf,
           &jpegSize,
           TJSAMP_444,  // full chroma (or TJSAMP_420 for smaller size)
           quality,
           TJFLAG_FASTDCT
       )
       != 0)
    {
        fprintf(stderr, "tjCompress2 error: %s\n", tjGetErrorStr());
        tjDestroy(handle);
        fclose(outfile);
        return -1;
    }

    // Write JPEG buffer to file
    fwrite(jpegBuf, jpegSize, 1, outfile);
    fclose(outfile);

    tjFree(jpegBuf);  // free buffer allocated by TurboJPEG
    tjDestroy(handle);

    return 0;
}

void print_usage(const char* argv_0)
{
    fprintf(
        stderr, "Usage: %s downscaling_factor input.jpg output.jpg [jpeg_quality]\n\ndownscaling_factor must be 1, 2, 4 or 8\njpeg_quality must be between 1 and 100.", argv_0
    );
}

int main(int argc, char* argv[])
{
    if(argc < 4)
    {
        print_usage(argv[0]);
        return 1;
    }

    int downscaling_factor = atoi(argv[1]);
    if(downscaling_factor != 1 && downscaling_factor != 2 && downscaling_factor != 4 && downscaling_factor != 8)
    {
        print_usage(argv[0]);
        return 1;
    }

    const char* inputFile  = argv[2];
    const char* outputFile = argv[3];


    int jpeg_quality = 90;

    if(argc == 5)
    {
        jpeg_quality = atoi(argv[4]);
        if(jpeg_quality < 1 || jpeg_quality > 100)
        {
            print_usage(argv[0]);
            return 1;
        }
    }

    unsigned long jpegSize = 0;
    unsigned char* jpegBuf = load_file(inputFile, &jpegSize);
    if(!jpegBuf)
    {
        return 1;
    }

    tjhandle tjInstance = tjInitDecompress();

    int width, height, subsamp, colorspace;
    if(tjDecompressHeader3(tjInstance, jpegBuf, jpegSize, &width, &height, &subsamp, &colorspace) != 0)
    {
        fprintf(stderr, "TurboJPEG error: %s\n", tjGetErrorStr());
        free(jpegBuf);
        tjDestroy(tjInstance);
        return 1;
    }

    printf("Original: %dx%d\n", width, height);

    // Example: pick 1/2 scaling factor if available
    tjscalingfactor sf = {1, downscaling_factor};
    int scaledWidth    = TJSCALED(width, sf);
    int scaledHeight   = TJSCALED(height, sf);

    printf("Downscaled: %dx%d\n", scaledWidth, scaledHeight);

    unsigned char* dstBuf = malloc(scaledWidth * scaledHeight * 3);
    if(!dstBuf)
    {
        perror("malloc");
        free(jpegBuf);
        tjDestroy(tjInstance);
        return 1;
    }

    if(tjDecompress2(tjInstance, jpegBuf, jpegSize, dstBuf, scaledWidth, 0 /* pitch */, scaledHeight, TJPF_RGB, 0) != 0)
    {
        fprintf(stderr, "Decompress error: %s\n", tjGetErrorStr());
        free(jpegBuf);
        free(dstBuf);
        tjDestroy(tjInstance);
        return 1;
    }

    save_jpeg(outputFile, dstBuf, scaledWidth, scaledHeight, jpeg_quality);

    free(jpegBuf);
    free(dstBuf);
    tjDestroy(tjInstance);

    printf("Saved downscaled image to %s\n", outputFile);
    return 0;
}
