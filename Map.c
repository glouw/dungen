#include "Map.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define toss(t, n) ((t*) malloc((n) * sizeof(t)))

#define zero(a) (memset(&(a), 0, sizeof(a)))

typedef int (*const Direction)(const void*, const void*);

typedef struct
{
    float x;
    float y;
}
Point;

typedef struct
{
    Point* point;
    int count;
    int max;
}
Points;

typedef struct
{
    Point a;
    Point b;
    Point c;
}
Tri;

typedef struct
{
    Tri* tri;
    int count;
    int max;
}
Tris;

typedef struct
{
    Point zer;
    Point one;
}
Flags;

static void bomb(const char* const message, ...)
{
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    exit(1);
}

static char** reset(char** block, const int rows, const int cols, const int blok)
{
    for(int row = 0; row < rows; row++)
        for(int col = 0; col < cols; col++)
            block[row][col] = blok;
    return block;
}

static char** new(const int rows, const int cols, const int blok)
{
    char** block = toss(char*, rows);
    for(int row = 0; row < rows; row++)
        block[row] = toss(char, cols);
    return reset(block, rows, cols, blok);
}

static Map mnew(const int rows, const int cols)
{
    Map map;
    zero(map);
    map.rows = rows;
    map.cols = cols;
    map.walling = new(map.rows, map.cols, '#');
    return map;
}

static int psame(const Point a, const Point b)
{
    return a.x == b.x && a.y == b.y;
}

static int fl(const float x)
{
    return (int) x - (x < (int) x);
}

static Point snap(const Point a, const int grid)
{
    const Point out = {
        (float) fl(a.x / grid) * grid,
        (float) fl(a.y / grid) * grid,
    };
    return out;
}

static Points psnew(const int max)
{
    const Points ps = { toss(Point, max), 0, max };
    return ps;
}

static Points psadd(Points ps, const Point p)
{
    if(ps.count == ps.max)
        bomb("points size limitation reached\n");
    ps.point[ps.count++] = p;
    return ps;
}

static int psfind(const Points ps, const Point p)
{
    for(int i = 0; i < ps.count; i++)
        if(psame(ps.point[i], p))
            return true;
    return false;
}

static Tris tsnew(const int max)
{
    const Tris ts = { toss(Tri, max), 0, max };
    return ts;
}

static Tris tsadd(Tris tris, const Tri tri)
{
    if(tris.count == tris.max)
        bomb("tris size limitation reached\n");
    tris.tri[tris.count++] = tri;
    return tris;
}

static int reveql(const Tri a, const Tri b)
{
    return psame(a.a, b.b) && psame(a.b, b.a);
}

static int foreql(const Tri a, const Tri b)
{
    return psame(a.a, b.a) && psame(a.b, b.b);
}

static int alligned(const Tri a, const Tri b)
{
    return foreql(a, b) || reveql(a, b);
}

// Flags alligned edges.
static void emark(Tris edges, const Flags flags)
{
    for(int i = 0; i < edges.count; i++)
    {
        const Tri edge = edges.tri[i];
        for(int j = 0; j < edges.count; j++)
        {
            if(i == j)
                continue;
            const Tri other = edges.tri[j];
            if(alligned(edge, other))
                edges.tri[j].c = flags.one;
        }
    }
}

// Creates new triangles from unique edges and appends to tris.
static Tris ejoin(Tris tris, const Tris edges, const Point p, const Flags flags)
{
    for(int j = 0; j < edges.count; j++)
    {
        const Tri edge = edges.tri[j];
        if(psame(edge.c, flags.zer))
        {
            const Tri tri = { edge.a, edge.b, p };
            tris = tsadd(tris, tri);
        }
    }
    return tris;
}

static int outob(const Point p, const int w, const int h)
{
    return p.x < 0 || p.y < 0 || p.x >= w || p.y >= h;
}

static int incircum(const Tri t, const Point p)
{
    const float ax = t.a.x - p.x;
    const float ay = t.a.y - p.y;
    const float bx = t.b.x - p.x;
    const float by = t.b.y - p.y;
    const float cx = t.c.x - p.x;
    const float cy = t.c.y - p.y;
    const float det =
        (ax * ax + ay * ay) * (bx * cy - cx * by) -
        (bx * bx + by * by) * (ax * cy - cx * ay) +
        (cx * cx + cy * cy) * (ax * by - bx * ay);
    return det > 0.0f;
}

// Collects all edges from given triangles.
static Tris ecollect(Tris edges, const Tris in, const Flags flags)
{
    for(int i = 0; i < in.count; i++)
    {
        const Tri tri = in.tri[i];
        const Tri ab = { tri.a, tri.b, flags.zer };
        const Tri bc = { tri.b, tri.c, flags.zer };
        const Tri ca = { tri.c, tri.a, flags.zer };
        edges = tsadd(edges, ab);
        edges = tsadd(edges, bc);
        edges = tsadd(edges, ca);
    }
    return edges;
}

static Tris delaunay(const Points ps, const int w, const int h, const int max, const Flags flags)
{
    Tris in = tsnew(max);
    Tris out = tsnew(max);
    Tris tris = tsnew(max);
    Tris edges = tsnew(max);
    // Shallow copies are exploited here for quick array concatentations.
    // In doing so, the original tris triangle is lost. This dummy pointer
    // will keep track of it for freeing at a later date.
    Tri* dummy = tris.tri;
    // The super triangle will snuggley fit over the screen.
    const Tri super = { { (float) -w, 0.0f }, { 2.0f * w, 0.0f }, { w / 2.0f, 2.0f * h } };
    tris = tsadd(tris, super);
    for(int j = 0; j < ps.count; j++)
    {
        in.count = out.count = edges.count = 0;
        const Point p = ps.point[j];
        // For all triangles...
        for(int i = 0; i < tris.count; i++)
        {
            const Tri tri = tris.tri[i];
            // Get triangles where point lies inside their circumcenter...
            if(incircum(tri, p))
                in = tsadd(in, tri);
            // And get triangles where point lies outside of their circumcenter.
            else out = tsadd(out, tri);
        }
        // Collect all triangle edges where point was inside circumcenter.
        edges = ecollect(edges, in, flags);
        // Flag edges that are non-unique.
        emark(edges, flags);
        // Construct new triangles with unique edges.
        out = ejoin(out, edges, p, flags);
        // Update triangle list.
        // FAST SHALLOW COPY - ORIGINAL POINTER LOST.
        tris = out;
    }
    free(dummy);
    free(in.tri);
    free(edges.tri);
    return tris;
}

static Points prand(const int w, const int h, const int max, const int grid, const int border)
{
    Points ps = psnew(max);
    for(int i = ps.count; i < ps.max; i++)
    {
        const Point p = {
            (float) (rand() % (w - border) + border / 2),
            (float) (rand() % (h - border) + border / 2),
        };
        const Point snapped = snap(p, grid);
        ps = psadd(ps, snapped);
    }
    return ps;
}

static Point sub(const Point a, const Point b)
{
    const Point out = { a.x - b.x, a.y - b.y };
    return out;
}

static float mag(const Point a)
{
    return sqrtf(a.x * a.x + a.y * a.y);
}

static float len(const Tri edge)
{
    return mag(sub(edge.b, edge.a));
}

static int descending(const void* a, const void* b)
{
    const Tri ea = *(const Tri*) a;
    const Tri eb = *(const Tri*) b;
    return len(ea) < len(eb) ? 1 : len(ea) > len(eb) ? -1 : 0;
}

static void sort(const Tris edges, const Direction direction)
{
    qsort(edges.tri, edges.count, sizeof(Tri), direction);
}

static int connected(const Point a, const Point b, const Tris edges, const Flags flags)
{
    Points todo = psnew(edges.max);
    Points done = psnew(edges.max);
    Tris reach = tsnew(edges.max);
    todo = psadd(todo, a);
    int connection = false;
    while(todo.count != 0 && connection != true)
    {
        const Point removed = todo.point[--todo.count];
        done = psadd(done, removed);
        // Get reachable edges from current point.
        reach.count = 0;
        for(int i = 0; i < edges.count; i++)
        {
            const Tri edge = edges.tri[i];
            if(psame(edge.c, flags.one))
                continue;
            if(psame(edge.a, removed))
                reach = tsadd(reach, edge);
        }
        // For all reachable edges
        for(int i = 0; i < reach.count; i++)
        {
            // Was the destination reached?
            if(psame(reach.tri[i].b, b))
            {
                connection = true;
                break;
            }
            // Otherwise add point of reachable edge to todo list.
            if(!psfind(done, reach.tri[i].b))
                todo = psadd(todo, reach.tri[i].b);
        }
    }
    free(todo.point);
    free(reach.tri);
    free(done.point);
    return connection;
}

// Graph Theory Reverse Delete algorithm. Kruskal 1956.
static void revdel(Tris edges, const int w, const int h, const Flags flags)
{
    sort(edges, descending);
    for(int i = 0; i < edges.count; i++)
    {
        Tri* edge = &edges.tri[i];
        if(outob(edge->a, w, h)
                || outob(edge->b, w, h))
        {
            edge->c = flags.one;
            continue;
        }
        // Break the connection.
        edge->c = flags.one;
        // If two points are not connected in anyway then reconnect.
        // Occasionally it will create a loop because true connectivity
        // checks all edges. Thankfully, the occasional loop benefits
        // the dungeon design else the explorer will get bored dead end after dead end.
        if(!connected(edge->a, edge->b, edges, flags)) edge->c = flags.zer;
    }
}

static void mdups(const Tris edges, const Flags flags)
{
    for(int i = 0; i < edges.count; i++)
        for(int j = 0; j < edges.count; j++)
        {
            if(psame(edges.tri[j].c, flags.one))
                continue;
            if(psame(edges.tri[i].c, flags.one))
                continue;
            if(reveql(edges.tri[i], edges.tri[j]))
                edges.tri[j].c = flags.one;
        }
}

static void mroom(const Map map, const Point where, const int w, const int h)
{
    for(int i = -w; i <= w; i++)
        for(int j = -h; j <= h; j++)
        {
            const int xx = where.x + i;
            const int yy = where.y + j;
            map.walling[yy][xx] = ' ';
        }
}

static void mcorridor(const Map map, const Point a, const Point b)
{
    const Point step = sub(b, a);
    const Point delta = {
        step.x > 0.0f ? 1.0f : step.x < 0.0f ? -1.0f : 0.0f,
        step.y > 0.0f ? 1.0f : step.y < 0.0f ? -1.0f : 0.0f,
    };
    const int sx = abs(step.x);
    const int sy = abs(step.y);
    const int dx = delta.x;
    const int dy = delta.y;
    int x = a.x;
    int y = a.y;
    for(int i = 0; i < sx; i++) map.walling[y][x += dx] = ' ';
    for(int i = 0; i < sy; i++) map.walling[y += dy][x] = ' ';
}

// ############################################# This is what a bone looks like
// #            ################################ when generated from an edge.
// #            #####################          #
// #  r o o m      c o r r i d o r    r o o m  #
// #     A      #####################    B     #
// #            ################################
// #############################################
static void bone(const Map map, const Tri e, const int w, const int h)
{
    mroom(map, e.a, w, h);
    mroom(map, e.b, w, h);
    mcorridor(map, e.a, e.b);
}

// Carve all bones from solid map.
static void carve(const Map map, const Tris edges, const Flags flags, const int grid)
{
    for(int i = 0; i < edges.count; i++)
    {
        const Tri e = edges.tri[i];
        if(psame(e.c, flags.one))
            continue;
        // Min room size ensures room will not be smaller than min x min.
        const int min = 2;
        const int size = grid / 2 - min;
        const int w = min + rand() % size;
        const int h = min + rand() % size;
        bone(map, e, w, h);
    }
}

Map xmgen(const int w, const int h, const int grid, const int max)
{
    srand(time(0));
    const Flags flags = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
    const int border = 2 * grid;
    const Points ps = prand(w, h, max, grid, border);
    const Tris tris = delaunay(ps, w, h, 9 * max, flags);
    const Tris edges = ecollect(tsnew(27 * max), tris, flags);
    revdel(edges, w, h, flags);
    const Map map = mnew(h, w);
    mdups(edges, flags);
    carve(map, edges, flags, grid);
    free(tris.tri);
    free(ps.point);
    free(edges.tri);
    return map;
}

void xmclose(const Map map)
{
    for(int row = 0; row < map.rows; row++)
        free(map.walling[row]);
    free(map.walling);
}

void xmprint(const Map map)
{
    for(int row = 0; row < map.rows; row++)
    for(int col = 0; col < map.cols; col++)
        printf("%c%s", map.walling[row][col], col == map.cols - 1 ? "\n" : "");
    putchar('\n');
}
