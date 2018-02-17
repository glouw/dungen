#include "Map.h"

int main()
{
    const Map map = xmgen(80, 120, 10, 30);
    xmprint(map);
    xmclose(map);
}
