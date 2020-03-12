#ifndef NO_GRID
#ifndef NO_GRID_PATHFIND
#include "SwanLib.hpp"
#include "../vm/Grid.hpp"
#include "../vm/List.hpp"
#include "../vm/Tuple.hpp"
#include "../vm/HasherAndEqualler.hpp"
#include<unordered_set>
#include<cmath>
#include "../../include/cpprintf.hpp"
#include<boost/heap/fibonacci_heap.hpp>
using namespace std;

struct point;
typedef boost::heap::fibonacci_heap<point, boost::heap::compare<std::function<bool(const point&, const point&)>>> heap;
typedef heap::handle_type heap_handle;

struct point {
int x, y;
point(int x1=0, int y1=0): x(x1), y(y1) {}
inline double length() const { return sqrt(x*x+y*y); }
};
inline bool operator== (const point& a, const point& b) { return a.x==b.x && a.y==b.y; }
inline bool operator!= (const point& a, const point& b) { return !(a==b); }
inline point operator+ (const point& a, const point& b) { return {a.x+b.x, a.y+b.y}; }
inline point operator- (const point& a, const point& b) { return {a.x-b.x, a.y-b.y}; }

struct PointHasher {
inline size_t operator() (const point& p) const { return (p.x<<16) | p.y; }
};

struct AStarPointInfo {
static const constexpr double maxval = 1e9;
double f, g, h;
point pt, parent;
int dir;
heap_handle handle;
bool handleInitialized;
AStarPointInfo(double f1=maxval, double g1=maxval, double h1=maxval, const point& x=point(-1,-1), const point& p=point(-1,-1), int d=-1, heap_handle hh = heap_handle(), bool hi=false): f(f1), g(g1), h(h1), pt(x), parent(p), dir(d), handle(hh), handleInitialized(hi)  {}
};

static void readPoints (QFiber& f, int& x1, int& y1, int& x2, int& y2, QV& value, QGrid& g) {
Swan::Range h(x1, x2, 1, false), v(y1, y2, 1, false);
if (f.getArgCount()==6) {
x1 = f.getNum(1); y1 = f.getNum(2); x2 = f.getNum(3); y2 = f.getNum(4);
h = Swan::Range(x1, x2, 1, false);
v = Swan::Range(y1, y2, 1, false);
value = f.at(5);
}
else if (f.getArgCount()==4) {
value = f.at(3);
if (f.isRange(1)) h = f.getRange(1);
else {
x1=x2=f.getNum(1);
h = Swan::Range(x1, x2, 1, true);
}
if (f.isRange(2)) v = f.getRange(2);
else {
y1=y2=f.getNum(2);
v = Swan::Range(y1, y2, 1, true);
}}
h.makeBounds(g.width, x1, x2);
v.makeBounds(g.height, y1, y2);
}

static void gridFillRect (QFiber& f) {
QGrid& g = f.getObject<QGrid>(0);
int x1=0, y1=0, x2=g.width, y2=g.height; QV value=QV::UNDEFINED;
if (f.getArgCount()==2) value=f.at(1);
else readPoints(f, x1, y1, x2, y2, value, g);
if (x1<0) x1+=g.width+1; if (x2<0) x2+=g.width+1;
if (y1<0) y1+=g.height+1; if (y2<0) y2+=g.height+1;
g.makeBounds(x1, y1);
g.makeBounds(x2, y2);
if (x1==x2) x2++;
if (y1==y2) y2++;
g.makeBounds(x1, y1);
g.makeBounds(x2, y2);
for (int y=y1; y<y2; y++) {
for (int x=x1; x<x2; x++) {
g.at(x, y) = value;
}}
}

static void gridDrawLine (QFiber& f) {
QGrid& g = f.getObject<QGrid>(0);
int x1=0, y1=0, x2=0, y2=0; QV value=0;
readPoints(f, x1, y1, x2, y2, value, g);
g.makeBounds(x1, y1);
g.makeBounds(x2, y2);
int e, dx = x2-x1, dy=y2-y1, incX=dx>0?1:-1, incY=dy>0?1:-1;
g.at(x1, y1) = value;
if (dx==0 && dy==0) return;
else if (dy==0) { // Horizontal line
while(x1!=x2) g.at(x1+=incX, y1) = value;
}
else if (dx==0) { // Vertical line
while(y1!=y2) g.at(x1, y1+=incY) =  value;
}
else if (abs(dx)>=abs(dy)) { // Line being more horizontal than vertical
dx = abs(dx*2);
dy = abs(dy*2);
e = (dx/-2);
while(x1!=x2) {
x1+=incX;
e+=dy;
if (e>=0) {
e-=dx;
y1+=incY;
}
g.at(x1, y1) = value;
}}
else if (abs(dx)<=abs(dy)) { // Line being more vertical than horizontal
dx = abs(dx*2);
dy = abs(dy*2);
e = (dy/-2);
while(y1!=y2) {
y1+=incY;
e+=dx;
if (e>=0) {
e-=dy;
x1+=incX;
}
g.at(x1, y1) = value;
}}
}

template <class F>
static void doFloodFill (QGrid& g, int x, int y, const F& eq, QV fill, QV src) {
if (x<0 || x>=g.width || y<0 || y>=g.height) return;
auto& dst = g.at(x,y);
if (!eq(src, dst)) return;
dst = fill;
doFloodFill(g, x+1, y, eq, fill, src);
doFloodFill(g, x, y+1, eq, fill, src);
doFloodFill(g, x -1, y, eq, fill, src);
doFloodFill(g, x, y -1, eq, fill, src);
}

static void gridFloodFill (QFiber& f) {
QGrid& g = f.getObject<QGrid>(0);
int x = f.getNum(1), y = f.getNum(2);
QV fill = f.at(3);
g.makeBounds(x, y);
if (x<0 || y<0 || x>=g.width || y>=g.height) return;
if (f.getArgCount()>=5) doFloodFill(g, x, y, QVBinaryPredicate(f.vm, f.at(4)), fill, g.at(x, y));
else doFloodFill(g, x, y, QVEqualler(f.vm), fill, g.at(x, y));
}

static void gridPathFind (QFiber& f) {
QGrid& g = f.getObject<QGrid>(0);
point src = { static_cast<int>(f.getNum(1)), static_cast<int>(f.getNum(2)) }, dest = { static_cast<int>(f.getNum(3)), static_cast<int>(f.getNum(4)) };
g.makeBounds(src.x, src.y);
g.makeBounds(dest.x, dest.y);
if (src.x<0 || src.y<0 || dest.x<0 || dest.y<0 || src.x>=g.width || src.y>=g.height || dest.x>=g.width || dest.y>=g.height) {
f.returnValue(QV::UNDEFINED);
return;
}
QV callback = f.at(5);
double diagCost = f.getOptionalNum(8, "diagonals", 0), turnCost = f.getOptionalNum(9, "turns", 0);
bool asDirList = f.getOptionalBool(7, "asDirectionList", false);
vector<point> neighbors;
if (diagCost>0) neighbors  = { {0,1}, {1,0}, {0,-1}, {-1,0}, {1,1}, {1,-1}, {-1,-1}, {-1,1}  };
else neighbors  = { {0,1}, {1,0}, {0,-1}, {-1,0} };
unordered_map<point, AStarPointInfo, PointHasher> info;
unordered_set<point, PointHasher> closedList;
auto comparator = [&](const point& a, const point& b){ return info[a].f > info[b].f; };
heap openList(comparator);
auto& srcInfo = (info[src]  = AStarPointInfo(0, 0, 0, src, src));
srcInfo.handle = openList.push(src);
srcInfo.handleInitialized = true;
point p;
int nNeighbors = neighbors.size();
while(!openList.empty()) {
p = openList.top();
openList.pop();
closedList.insert(p);
if (p==dest) break;
for (int d=0; d<nNeighbors; d++) {
const point& dp = neighbors[d];
point next = p+dp;
if (next.x<0 || next.y<0 || next.x>=g.width || next.y>=g.height) continue;
if (closedList.find(next)!=closedList.end()) continue; 
f.pushCppCallFrame();
f.push(callback);
f.push(g.at(next.x, next.y));
f.push(&g);
f.push(static_cast<double>(next.x));
f.push(static_cast<double>(next.y));
f.push(static_cast<double>(p.x));
f.push(static_cast<double>(p.y));
f.call(6);
QV re = f.at(-1);
double cost;
if (re.isNum()) cost = re.asNum();
else if (re.isFalse() || re.isNullOrUndefined()) cost = 0;
else cost=1;
f.pop();
f.popCppCallFrame();
if (cost<1) continue;
auto& pInfo  = info[p];
auto& nextInfo = info[next];
point diff = dest-next;
double G = pInfo.g + cost*(d>=4? diagCost:1) + (d!=pInfo.dir? turnCost:0);
double H = diff.length();
double F = G+H;
if (nextInfo.f<F) continue;
nextInfo = { F, G, H, next, p, d, nextInfo.handle, nextInfo.handleInitialized  };
if (nextInfo.handleInitialized) openList.increase(nextInfo.handle);
else { nextInfo.handle = openList.push(next); nextInfo.handleInitialized=true; }
}//going through neighbors
}//going through openList
if (p!=dest) { // No path found
f.returnValue(QV::UNDEFINED);
return;
}
vector<point> path;
int count=0, dir=-1;
p = dest;
while(p!=src) {
if (!asDirList)  path.push_back(p);
else if (dir==info[p].dir) count++;
else {
if (count>0) path.push_back(point(count, dir));
dir=info[p].dir;
count=1;
}
p = info[p] .parent;
}
if (asDirList && count>0) path.push_back(point(count, dir));
reverse(path.begin(), path.end());
QList* list = f.vm.construct<QList>(f.vm);
f.returnValue(list);
for (auto& p: path) {
QV t[] = { static_cast<double>(p.x), static_cast<double>(p.y) };
list->data.push_back(QTuple::create(f.vm, 2, t));
}
}

static void gridTestDirectPath (QFiber& f) {
QGrid& g = f.getObject<QGrid>(0);
int x1 = f.getNum(1), y1 = f.getNum(2), x2 = f.getNum(3), y2 = f.getNum(4);
QV callback = f.at(5);
g.makeBounds(x1, y1);
g.makeBounds(x2, y2);
int e, dx = x2-x1, dy=y2-y1, incX=dx>0?1:-1, incY=dy>0?1:-1, dirX=incX>0?1:3, dirY=incY>0?0:2;
const point dirs[4] = { {0,1}, {1,0}, {0,-1}, {-1,0} };
auto isWall = [&](int x, int y, int d)->bool{
if (x<0 || y<0 || x>=g.width || y>=g.height) return true;
point pt(x,y), dir=dirs[d], prev=pt-dir;
f.pushCppCallFrame();
f.push(callback);
f.push(g.at(pt.x, pt.y));
f.push(&g);
f.push(static_cast<double>(pt.x));
f.push(static_cast<double>(pt.y));
f.push(static_cast<double>(prev.x));
f.push(static_cast<double>(prev.y));
f.call(6);
QV re = f.at(-1);
f.pop();
f.popCppCallFrame();
if (re.isBool()) return !re.asBool();
else return re.asNum()<1;
};
#define RET(X,Y,B) { QV t[] = { B, static_cast<double>(X), static_cast<double>(Y) }; f.returnValue(QTuple::create(f.vm, 3, t)); return; }
if (dx==0 && dy==0) RET(x1, y1, true)
else if (dy==0) { // Horizontal line
while(x1!=x2 && !isWall(x1, y1, dirX)) x1+=incX;
RET(x1, y1, x1==x2)
}
else if (dx==0) { // Vertical line
while(y1!=y2 && !isWall(x1, y1, dirY)) y1+=incY;
RET(x1, y1, y1==y2)
}
else if (abs(dx)>=abs(dy)) { // Line being more horizontal than vertical
dx = abs(dx*2);
dy = abs(dy*2);
e = dx/-2;
while(x1!=x2) {
x1+=incX;
e+=dy;
if (e>=0) {
e-=dx;
y1+=incY;
}
if (isWall(x1,y1, dirX) || isWall(x1,y1, dirY)) {
RET(x1, y1, false)
}}}
else if (abs(dx)<=abs(dy)) { // Line being more vertical than horizontal
dx = abs(dx*2);
dy = abs(dy*2);
e = dy/-2;
while(y1!=y2) {
y1+=incY;
e+=dx;
if (e>=0) {
e-=dy;
x1+=incX;
}
if (isWall(x1,y1, dirX) || isWall(x1,y1, dirY)) {
RET(x1, y1, false)
}}}
RET(x1, y1, true)
#undef RET
}

void initGridPathfind (QVM& vm) {
vm.gridClass
->bind("fill", gridFillRect)
->bind("draw", gridDrawLine)
->bind("floodFill", gridFloodFill)
->bind("pathfind", gridPathFind)
->bind("hasDirectPath", gridTestDirectPath)
;
}



#endif
#endif
