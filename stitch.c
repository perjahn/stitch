#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

int SplitToArray(char *text, unsigned expectedElements, unsigned elements[], char *errorMessage);
int DoStuff(unsigned width, unsigned height, char **inputfiles, unsigned fileCount, char *outputfile, unsigned overlaps[]);

int main(int argc, char *argv[])
{
    if (argc < 6)
    {
        printf("Usage: stitch <width> <height> <inputfiles...> <outputfile> <overlaps>\n");
        return 1;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    if (width < 1)
    {
        printf("Width must be atleast 1.\n");
        return 1;
    }
    if (height < 1)
    {
        printf("Height must be atleast 1.\n");
        return 1;
    }
    if (width * height < 2)
    {
        printf("Width*Height must be atleast 2.\n");
        return 1;
    }
    if (width * height != argc - 5)
    {
        printf("Width*Height must match number of files (expected %d, got %d).\n", width * height, argc - 5);
        return 1;
    }

    char **inputfiles = argv + 3;
    unsigned fileCount = argc - 5;
    char *outputfile = argv[argc - 2];
    char *overlapsArg = argv[argc - 1];

    unsigned overlaps[width + height - 2];
    char errorMessage[1000];
    int result = SplitToArray(overlapsArg, width + height - 2, overlaps, errorMessage);
    if (result)
    {
        printf("Invalid overlaps (%s).\n", errorMessage);
        return result;
    }

    result = DoStuff(width, height, inputfiles, fileCount, outputfile, overlaps);

    return result;
}

int SplitToArray(char *text, unsigned expectedElements, unsigned elements[], char *errorMessage)
{
    unsigned commas = 0;
    unsigned length = strlen(text);
    for (int i = 0; i < length; i++)
    {
        if ((text[i] < '0' || text[i] > '9') && text[i] != ',')
        {
            sprintf(errorMessage, "expected number or comma, got '%c'", text[i]);
            return 1;
        }
        if (text[i] == ',')
        {
            commas++;
        }
    }

    if (commas + 1 != expectedElements)
    {
        sprintf(errorMessage, "expected %d comma separated values, got %d", expectedElements, commas + 1);
        return 1;
    }

    char buf[100];
    char *p, *p2;
    int j = 0;
    int value;
    for (p = p2 = text; *p; p++)
    {
        if (*p == ',')
        {
            if (p2 - p > 99)
            {
                sprintf(errorMessage, "Too big value: %ld\n", p - p2);
                return 1;
            }

            memcpy(buf, p2, p - p2);
            buf[p - p2] = 0;
            value = atoi(buf);
            elements[j++] = value;

            p2 = p + 1;
        }
    }

    memcpy(buf, p2, p - p2);
    buf[p - p2] = 0;
    value = atoi(buf);
    elements[j++] = value;

    return 0;
}

int ReadFile(char *filename, unsigned char **buf, unsigned long *bufsize)
{
    printf("Reading: '%s'\n", filename);

    FILE *in;
    if (!(in = fopen(filename, "r")))
    {
        printf("Couldn't open file: '%s'\n", filename);
        return 1;
    }

    fseek(in, 0, SEEK_END);
    *bufsize = ftell(in);

    *buf = malloc(*bufsize);
    if (!*buf)
    {
        printf("Out of memory: %ld", *bufsize);
        return 1;
    }

    fseek(in, 0, SEEK_SET);
    long bla = fread(*buf, *bufsize, 1, in);

    fclose(in);

    return 0;
}

void FlipVertically(unsigned char *buf, unsigned height, unsigned stride)
{
    unsigned char *tmpbuf = malloc(stride);

    for (int y = 0; y < height / 2; y++)
    {
        unsigned char *p1 = buf + y * stride;
        unsigned char *p2 = buf + (height - 1 - y) * stride;
        memcpy(tmpbuf, p1, stride);
        memcpy(p1, p2, stride);
        memcpy(p2, tmpbuf, stride);
    }

    free(tmpbuf);
}

void PaintIntoCanvas(unsigned char **buffers, unsigned bufCount, unsigned *widths, unsigned *heights, unsigned *strides, unsigned *offsetsx, unsigned *offsetsy,
                     unsigned char *canvasbuf, unsigned canvaswidth, unsigned canvasheight, unsigned canvasstride,
                     unsigned *overlaps)
{
    for (unsigned y = 0; y < canvasheight; y++)
    {
        for (unsigned x = 0; x < canvaswidth; x++)
        {
            unsigned layers[bufCount];
            unsigned layerCount = 0;

            for (unsigned i = 0; i < bufCount; i++)
            {
                if (x >= offsetsx[i] && x < offsetsx[i] + widths[i] && y >= offsetsy[i] && y < offsetsy[i] + heights[i])
                {
                    layers[layerCount++] = i;
                }
            }

            unsigned short r = 0;
            unsigned short g = 0;
            unsigned short b = 0;

            unsigned char *p;

            if (layerCount == 2)
            {
                if (layers[0] + 1 == layers[1])
                {
                    p = buffers[layers[0]] + (y - offsetsy[layers[0]]) * strides[layers[0]] + (x - offsetsx[layers[0]]) * 3;
                    unsigned short r1 = *p;
                    unsigned short g1 = *(p + 1);
                    unsigned short b1 = *(p + 2);

                    p = buffers[layers[1]] + (y - offsetsy[layers[1]]) * strides[layers[1]] + (x - offsetsx[layers[1]]) * 3;
                    unsigned short r2 = *p;
                    unsigned short g2 = *(p + 1);
                    unsigned short b2 = *(p + 2);

                    unsigned o1 = offsetsx[layers[0]];
                    unsigned w1 = widths[layers[0]];
                    unsigned o2 = offsetsx[layers[1]];

                    double factor2 = (x - o2) / 1.0 / (o1 + w1 - o2 - 1);
                    double factor1 = 1 - factor2;

                    r = round(r1 * factor1 + r2 * factor2);
                    g = round(g1 * factor1 + g2 * factor2);
                    b = round(b1 * factor1 + b2 * factor2);
                }
                else
                {
                    p = buffers[layers[0]] + (y - offsetsy[layers[0]]) * strides[layers[0]] + (x - offsetsx[layers[0]]) * 3;
                    unsigned short r1 = *p;
                    unsigned short g1 = *(p + 1);
                    unsigned short b1 = *(p + 2);

                    p = buffers[layers[1]] + (y - offsetsy[layers[1]]) * strides[layers[1]] + (x - offsetsx[layers[1]]) * 3;
                    unsigned short r2 = *p;
                    unsigned short g2 = *(p + 1);
                    unsigned short b2 = *(p + 2);

                    unsigned o1 = offsetsy[layers[0]];
                    unsigned h1 = heights[layers[0]];
                    unsigned o2 = offsetsy[layers[1]];

                    double factor2 = (y - o2) / 1.0 / (o1 + h1 - o2 - 1);
                    double factor1 = 1 - factor2;

                    r = round(r1 * factor1 + r2 * factor2);
                    g = round(g1 * factor1 + g2 * factor2);
                    b = round(b1 * factor1 + b2 * factor2);
                }
            }
            else if (layerCount == 4)
            {
                p = buffers[layers[0]] + (y - offsetsy[layers[0]]) * strides[layers[0]] + (x - offsetsx[layers[0]]) * 3;
                unsigned short r1 = *p;
                unsigned short g1 = *(p + 1);
                unsigned short b1 = *(p + 2);

                p = buffers[layers[1]] + (y - offsetsy[layers[1]]) * strides[layers[1]] + (x - offsetsx[layers[1]]) * 3;
                unsigned short r2 = *p;
                unsigned short g2 = *(p + 1);
                unsigned short b2 = *(p + 2);

                p = buffers[layers[2]] + (y - offsetsy[layers[2]]) * strides[layers[2]] + (x - offsetsx[layers[2]]) * 3;
                unsigned short r3 = *p;
                unsigned short g3 = *(p + 1);
                unsigned short b3 = *(p + 2);

                p = buffers[layers[3]] + (y - offsetsy[layers[3]]) * strides[layers[3]] + (x - offsetsx[layers[3]]) * 3;
                unsigned short r4 = *p;
                unsigned short g4 = *(p + 1);
                unsigned short b4 = *(p + 2);

                unsigned ox1 = offsetsx[layers[0]];
                unsigned w1 = widths[layers[0]];
                unsigned ox2 = offsetsx[layers[1]];

                unsigned oy1 = offsetsy[layers[0]];
                unsigned h1 = heights[layers[0]];
                unsigned oy3 = offsetsy[layers[3]];

                double factor2 = (x - ox2) / 1.0 / (ox1 + w1 - ox2 - 1);
                double factor1 = 1 - factor2;
                double factor4 = (y - oy3) / 1.0 / (oy1 + h1 - oy3 - 1);
                double factor3 = 1 - factor4;

                r = round((r1 * factor1 + r2 * factor2) * factor3 + (r3 * factor1 + r4 * factor2) * factor4);
                g = round((g1 * factor1 + g2 * factor2) * factor3 + (g3 * factor1 + g4 * factor2) * factor4);
                b = round((b1 * factor1 + b2 * factor2) * factor3 + (b3 * factor1 + b4 * factor2) * factor4);
            }
            else if (layerCount == 1)
            {
                p = buffers[layers[0]] + (y - offsetsy[layers[0]]) * strides[layers[0]] + (x - offsetsx[layers[0]]) * 3;
                r = *p;
                g = *(p + 1);
                b = *(p + 2);
            }

            p = canvasbuf + y * canvasstride + x * 3;

            *p = r;
            *(p + 1) = g;
            *(p + 2) = b;
        }
    }
}

int SaveOutput(char *outputfile, unsigned char *buf, unsigned size)
{
    printf("Saving: '%s'\n", outputfile);

    FILE *out;
    if (!(out = fopen(outputfile, "w")))
    {
        printf("Couldn't open file: '%s'\n", outputfile);
        return 1;
    }

    long bla2 = fwrite(buf, size, 1, out);

    fclose(out);

    return 0;
}

int DoStuff(unsigned width, unsigned height, char **inputfiles, unsigned fileCount, char *outputfile, unsigned overlaps[])
{
    for (int i = 0; i < width + height - 2; i++)
    {
        printf("overl: %u\n", overlaps[i]);
    }

    printf("Width: %d, Height: %d, Inputfiles: %d\n", width, height, fileCount);
    printf("Stitching %d files to: '%s'\n", fileCount, outputfile);

    unsigned char *buffers[fileCount];
    unsigned long bufsizes[fileCount];

    for (int i = 0; i < fileCount; i++)
    {
        int result = ReadFile(inputfiles[i], &buffers[i], &bufsizes[i]);

        if (result)
        {
            return result;
        }
    }

    for (int i = 0; i < fileCount; i++)
    {
        if (bufsizes[i] < 54 ||
            buffers[i][0] != 'B' ||
            buffers[i][1] != 'M' ||
            buffers[i][10] != 54 ||
            buffers[i][14] != 40 ||
            buffers[i][26] != 1 ||
            buffers[i][28] != 24)
        {
            printf("Invalid image: '%s'\n", inputfiles[i]);
            return 1;
        }
    }

    unsigned char *pixbufs[fileCount];
    unsigned widths[fileCount];
    unsigned heights[fileCount];
    unsigned strides[fileCount];
    unsigned offsetsx[fileCount];
    unsigned offsetsy[fileCount];

    for (int i = 0; i < fileCount; i++)
    {
        unsigned char *buf = buffers[i];
        widths[i] = buf[18] + (buf[19] << 8);
        heights[i] = buf[22] + (buf[23] << 8);

        pixbufs[i] = buffers[i] + 54;
        strides[i] = widths[i] * 3;

        FlipVertically(pixbufs[i], heights[i], strides[i]);
    }

    unsigned canvaswidth = 0;
    unsigned canvasheight = 0;

    for (int i = 0; i < width; i++)
    {
        canvaswidth += widths[i];
    }
    for (int i = 1; i < width; i++)
    {
        canvaswidth -= overlaps[i - 1];
    }
    for (int i = 0; i < fileCount; i += width)
    {
        canvasheight += heights[i];
    }
    for (int i = 1; i < height; i++)
    {
        canvasheight -= overlaps[i + width - 2];
    }

    unsigned canvasstride = canvaswidth * 3;

    printf("Canvaswidth: %d, Canvasheight: %d\n", (int)canvaswidth, (int)canvasheight);

    unsigned canvasbufsize = canvasheight * canvasstride + 54;
    unsigned char *canvasbuf = malloc(canvasbufsize);
    memset(canvasbuf, 0, canvasbufsize);
    canvasbuf[0] = 'B';
    canvasbuf[1] = 'M';
    canvasbuf[2] = canvasbufsize & 0x000000FF;
    canvasbuf[3] = (canvasbufsize >> 8) & 0x000000FF;
    canvasbuf[4] = (canvasbufsize >> 16) & 0x000000FF;
    canvasbuf[5] = (canvasbufsize >> 24) & 0x000000FF;
    canvasbuf[10] = 54;
    canvasbuf[14] = 40;
    canvasbuf[18] = canvaswidth & 0x000000FF;
    canvasbuf[19] = (canvaswidth >> 8) & 0x000000FF;
    canvasbuf[22] = canvasheight & 0x000000FF;
    canvasbuf[23] = (canvasheight >> 8) & 0x000000FF;
    canvasbuf[26] = 1;
    canvasbuf[28] = 24;

    for (int i = 0; i < fileCount; i++)
    {
        unsigned offsetx = 0;
        unsigned offsety = 0;
        for (int j = 0; j < i % width; j++)
        {
            offsetx += widths[j];
            offsetx -= overlaps[j];
        }
        for (int j = 0; j < i / width; j++)
        {
            offsety += heights[j * 3];
            offsety -= overlaps[j + width - 1];
        }

        offsetsx[i] = offsetx;
        offsetsy[i] = offsety;
    }

    PaintIntoCanvas(pixbufs, fileCount, widths, heights, strides, offsetsx, offsetsy, canvasbuf + 54, canvaswidth, canvasheight, canvasstride, overlaps);

    FlipVertically(canvasbuf + 54, canvasheight, canvasstride);

    SaveOutput(outputfile, canvasbuf, canvasbufsize);

    for (int i = 0; i < fileCount; i++)
    {
        free(buffers[i]);
    }

    return 0;
}
