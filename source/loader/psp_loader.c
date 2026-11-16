#include "psp_loader.h"

#include <stdio.h>
#include <stdlib.h>

bool psp_loader_load(const char *path, PSP_Image *image)
{
    if (path == NULL || image == NULL)
        return false;

    image->data = NULL;
    image->size = 0;
    image->loaded = false;

    FILE *file = fopen(path, "rb");

    if (file == NULL)
        return false;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return false;
    }

    long file_size = ftell(file);

    if (file_size <= 0)
    {
        fclose(file);
        return false;
    }

    rewind(file);

    uint8_t *buffer = malloc((size_t)file_size);

    if (buffer == NULL)
    {
        fclose(file);
        return false;
    }

    size_t bytes_read = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size)
    {
        free(buffer);
        return false;
    }

    image->data = buffer;
    image->size = bytes_read;
    image->loaded = true;

    return true;
}

void psp_loader_unload(PSP_Image *image)
{
    if (image == NULL)
        return;

    free(image->data);

    image->data = NULL;
    image->size = 0;
    image->loaded = false;
}
