#include <SDL2/SDL.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct
{
    float x;
    float y;
}
Point;

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
    Point* point;
    int count;
    int max;
}
Points;

typedef int (*const Direction)(const void*, const void*);

const Point zer = { 0.0f, 0.0f };

const Point one = { 1.0f, 1.0f };

static Tris tsnew(const int max)
{
    const Tris ts = { (Tri*) malloc(sizeof(Tri) * max), 0, max };
    return ts;
}

static Tris tsadd(Tris tris, const Tri tri)
{
    if(tris.count == tris.max)
    {
        puts("tris size limitation reached");
        exit(1);
    }
    tris.tri[tris.count++] = tri;
    return tris;
}

static int peql(const Point a, const Point b)
{
    return a.x == b.x && a.y == b.y;
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
static Tris ecollect(Tris edges, const Tris in)
{
    for(int i = 0; i < in.count; i++)
    {
        const Tri tri = in.tri[i];
        const Tri ab = { tri.a, tri.b, zer };
        const Tri bc = { tri.b, tri.c, zer };
        const Tri ca = { tri.c, tri.a, zer };
        edges = tsadd(edges, ab);
        edges = tsadd(edges, bc);
        edges = tsadd(edges, ca);
    }
    return edges;
}

// Returns true if edge ab of two triangles are alligned.
static int alligned(const Tri a, const Tri b)
{
    return (peql(a.a, b.a) && peql(a.b, b.b)) || (peql(a.a, b.b) && peql(a.b, b.a));
}

// Flags alligned edges.
static Tris emark(Tris edges)
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
                edges.tri[j].c = one;
        }
    }
    return edges;
}

// Creates new triangles from unique edges and appends to tris.
static Tris ejoin(Tris tris, const Tris edges, const Point p)
{
    for(int j = 0; j < edges.count; j++)
    {
        const Tri edge = edges.tri[j];
        if(peql(edge.c, zer))
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

static Tris delaunay(const Points ps, const int w, const int h, const int max)
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
        edges = ecollect(edges, in);
        // Flag edges that are non-unique.
        edges = emark(edges);
        // Construct new triangles with unique edges.
        out = ejoin(out, edges, p);
        // Update triangle list.
        // FAST SHALLOW COPY - ORIGINAL POINTER LOST.
        tris = out;
    }
    free(dummy);
    free(in.tri);
    free(edges.tri);
    return tris;
}

static Points psnew(const int max)
{
    const Points ps = { (Point*) malloc(sizeof(Point) * max), 0, max };
    return ps;
}

static Points psadd(Points ps, const Point p)
{
    if(ps.count == ps.max)
    {
        puts("points size limitation reached");
        exit(1);
    }
    ps.point[ps.count++] = p;
    return ps;
}

static Points prand(const int w, const int h, const int max)
{
    Points ps = psnew(max);
    const int border = 100;
    for(int i = 0; i < max; i++)
    {
        const Point p = {
            (float) (rand() % (w - border) + border / 2),
            (float) (rand() % (h - border) + border / 2),
        };
        ps.point[ps.count++] = p;
    }
    return ps;
}

static Point sub(const Point a, const Point b)
{
    Point out;
    out.x = a.x - b.x;
    out.y = a.y - b.y;
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

static int psfind(const Points ps, const Point p)
{
    for(int i = 0; i < ps.count; i++)
        if(peql(ps.point[i], p))
            return 1;
    return 0;
}

static int connected(const Point a, const Point b, const Tris edges)
{
    Points todo = psnew(edges.max);
    Points done = psnew(edges.max);
    Tris reach = tsnew(edges.max);
    todo = psadd(todo, a);
    int connection = 0;
    while(todo.count != 0 && connection != 1)
    {
        const Point removed = todo.point[--todo.count];
        done = psadd(done, removed);
        // Get reachable edges from current point.
        reach.count = 0;
        for(int i = 0; i < edges.count; i++)
        {
            const Tri edge = edges.tri[i];
            if(peql(edge.c, one))
                continue;
            if(peql(edge.a, removed) || peql(edge.b, removed))
                reach = tsadd(reach, edge);
        }
        // For all reachable edges
        for(int i = 0; i < reach.count; i++)
        {
            const Point other = peql(reach.tri[i].a, removed) ? reach.tri[i].b : removed;
            // Destination reached.
            if(peql(other, b))
            {
                connection = 1;
                break;
            }
            // Otherwise add todo list.
            if(!psfind(done, other))
                todo = psadd(todo, other);
        }
    }
    free(todo.point);
    free(reach.tri);
    free(done.point);
    return connection;
}

static void revdel(Tris edges, const int w, const int h)
{
    sort(edges, descending);
    for(int i = 0; i < edges.count; i++)
    {
        Tri* edge = &edges.tri[i];
        if(outob(edge->a, w, h) || outob(edge->b, w, h))
        {
            edge->c = one;
            continue;
        }
        // Break the connection.
        edge->c = one;
        // If two points are not connected in anyway then reconnect.
        if(!connected(edge->a, edge->b, edges)) edge->c = zer;
    }
}

static void draw(SDL_Renderer* const renderer, const Tris edges)
{
    for(int i = 0; i < edges.count; i++)
    {
        const Tri e = edges.tri[i];
        if(peql(e.c, one))
            continue;
        printf("%f %f %f %f\n", e.a.x, e.a.y, e.b.x, e.b.y);
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF);
        SDL_RenderDrawLine(renderer, e.a.x, e.a.y, e.b.x, e.b.y);
    }
}

int main()
{
    srand(time(0));
    const int xres = 800;
    const int yres = 600;
    const int max = 100;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer(xres, yres, 0, &window, &renderer);
    const Points ps = prand(xres, yres, max);
    // 100 points = 300 points worth of triangles for spanning a delaunay network.
    const Tris tris = delaunay(ps, xres, yres, 3 * max);
    // 300 triangles = 900 points worth of non-unique edges.
    const Tris edges = ecollect(tsnew(9 * max), tris);
    // The revere-delete algorithm is a an algo in graph theory used to
    // obtain a minimum spanning tree from a given connected edge weighted graph.
    // Kruskal et al. (C) 1956.
    revdel(edges, xres, yres);
    draw(renderer, edges);
    SDL_RenderPresent(renderer);
    SDL_Event event;
    do
    {
        SDL_PollEvent(&event);
        SDL_Delay(10);
    }
    while(event.type != SDL_KEYUP && event.type != SDL_QUIT);
    free(tris.tri);
    free(ps.point);
    free(edges.tri);
    return 0;
}
