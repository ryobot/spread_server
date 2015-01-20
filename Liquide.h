// Liquide.h

#ifndef LIQUIDE_H
#define LIQUIDE_H

using namespace std;
#include <vector>

#define BLOCK_SIZE 4
#define AREA_WIDTH 160
#define AREA_HEIGHT 112
#define DROP_R 8
#define HOLE_R 4
#define HOLE_R2 20
#define NEIGHBOR 3
#define GAME_BLOCK 16

class Pos
{
public:
  int x;
  int y;

  Pos()
  {
    x = y = 0;
  }
  Pos(int _x, int _y)
  {
    x = _x;
    y = _y;
  }
  Pos operator +(Pos &pos)
  {
    Pos tmp;
    tmp.x = x + pos.x;
    tmp.y = y + pos.y;
    return tmp;
  }
  Pos& operator =(const Pos &pos)
  {
    x = pos.x;
    y = pos.y;
    return *this;
  }
  Pos& operator +=(Pos &pos)
  {
    x += pos.x;
    y += pos.y;
    return *this;
  }
  Pos& operator -=(Pos &pos)
  {
    x -= pos.x;
    y -= pos.y;
    return *this;
  }
};

class Drop
{
public:
  Pos pos;
  int maxR;
  int spread;
  bool eraseDrop;

  Drop(int posx, int posy, int r=DROP_R)
  {
    pos.x = posx;
    pos.y = posy;
    spread = 0;
    maxR = r;
    eraseDrop = false;
  }
  Drop(Pos &_pos, int r=DROP_R)
  {
    pos = _pos;
    spread = 0;
    maxR = r;
    eraseDrop = false;
  }
  Drop& operator =(const Drop &drop)
  {
    pos = drop.pos;
    maxR = drop.maxR;
    spread = drop.spread;
    eraseDrop = drop.eraseDrop;
    return *this;
  }
  void SetErase(bool erase)
  {
    eraseDrop = erase;
  }
  bool Grow()
  {
    if ( spread >= maxR )
      return false;
    spread++;
    return true;
  }
  bool IsGrowing()
  {
      return ( spread < maxR );
  }
};

class Hole
{
public:
  Pos pos;
  int count;
  int maxR;

  Hole(int posx, int posy, int r=HOLE_R)
  {
    pos.x = posx;
    pos.y = posy;
    count = 0;
    maxR = r;
  }
  Hole(Pos &_pos, int r=HOLE_R)
  {
    pos = _pos;
    count = 0;
    maxR = r;
  }
  bool IsInHole(int x, int y, int r)
  {
    if ( abs(pos.x - x) > r )
      return false;
    if ( abs(pos.y - y) > r )
      return false;
    float dx = pos.x - x;
    float dy = pos.y - y;
    if ( dx*dx + dy*dy > r*r )
      return false;
    return true;
  }
  int GetCountAndReset()
  {
    int ret = count;
    count = 0;
    return ret;
  }
};

class DistanceTable
{
private:
  int data[100][100];

public:
  DistanceTable();
  int Distance(int x, int y);
};

class RandomPositionTable
{
private:
  Pos pos[AREA_WIDTH*AREA_HEIGHT];
  int index;
  
public:
  RandomPositionTable();
  void Reset();
  Pos* Get();
  int GetIndex();
};

class Liquide
{
public:
	Liquide();
	~Liquide();

public:
  vector <Drop> drops;
  vector <Hole> holes;

  void AddDrop(int x, int y);
  Hole* AddHole(int x, int y);
  void EraseDrop(int x, int y);
  bool Update();
  void Reset();
  int PointInHole(int x, int y, int r=HOLE_R);
  int GetLiquideAmount();
  void SetShrink();
  void DumpNMap();
  int GameBlockPosX(int x);
  int GameBlockPosY(int y);
  
  vector <int> dividedGroup;
  
  int GetDropCountAndReset();
  char* GetDataString(char *str);
  char* GetGroupString(char *str);
  char* GetHoleString(char *str);
  char* GetDropString(char *str);
  int GetTime();
  
protected:
  bool AddPoint(int x, int y, int gNum=-1);
  void ResumeGroup(int gNum);
  bool Swell(int x, int y, Drop &drop);
  //int FloodGroup(int gNum, Pos& pos, int cnt);
  void FloodGroup(int gNum, Pos &pos);
  bool IsToBeFlood(int gNum, Pos *pos);
  int Flood(int gNum, Pos *pos, int cnt);
  bool LowPassEdge(int gNum, int count);
  bool ErasePoint(int x, int y);
  bool IsEdgeOfGroup(int gNum, Pos& pos);
  bool IsInnerEdgeOfGroup(int gNum, Pos& pos);
  int IsEdge(int posx, int posy, int max=9);
  bool DropSpread(Drop &drop);
  int FallIntoHole(int gNum);
  bool EdgeSearch(Pos &pos, Pos *next);
  int Shrink(int gNum);
  int Shrink2(int gNum);
  bool Shrink3();
  int ShrinkIntoHole(int gNum, int holeNum, Pos *startPos, int r);
  void Divisions();
  
  DistanceTable dt;
  int currentGroup;
  int dropCounter;
  unsigned int time;
  
  RandomPositionTable rpt;
  
protected:
  bool bMap[AREA_WIDTH][AREA_HEIGHT];
  int nMap[AREA_WIDTH][AREA_HEIGHT];
  int gMap[AREA_WIDTH][AREA_HEIGHT];
  bool notFixed, shrinked, allGroupsTouched;
  int holeNum;
};

#endif
