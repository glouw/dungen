#pragma once

typedef struct
{
    char** walling;
    int h;
    int w;
}
Map;

Map xmgen(const int w, const int h, const int grid, const int max);

void xmclose(const Map);

void xmprint(const Map);
