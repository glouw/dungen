#include "Map.h"

int main()
{
    const Map map = xmgen(80, 120, 10, 40);
    xmprint(map);
    xmclose(map);
}
