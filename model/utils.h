#ifndef UTILS_H
#define UTILS_H

unsigned int GetUint24(const unsigned char *data)
{
    // return *data + (*(data + 1) << 8) + (*(data + 2) << 16);
    return *(data + 2) + (*(data + 1) << 8) + (*(data) << 16);
}

int GetInt24(const unsigned char *data)
{
    // return *data + (*(data + 1) << 8) + (*(data + 2) << 16);
    return (int)(*(data + 2) + (*(data + 1) << 8) + (*(data) << 16));
}

unsigned int GetUint32(const unsigned char *data)
{
    // return *data + (*(data + 1) << 8) + (*(data + 2) << 16) + (*(data + 3) << 24);
    return *(data + 3) + (*(data + 2) << 8) + (*(data + 1) << 16) + (*(data) << 24);
}

#endif // UTILS_H
