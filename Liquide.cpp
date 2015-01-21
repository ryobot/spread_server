// Liquide.cpp

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "Liquide.h"

#define TEMPORARY_GROUP 10000

//////// Liquide ///////////////////////////

Liquide::Liquide()
{
  Reset();
}

Liquide::~Liquide()
{
}

bool Liquide::AddPoint(int x, int y, int gNum)
{
  if ( !bMap[x][y] )
    return false;
  bMap[x][y] = false;
  if ( gNum >= 0 )
    gMap[x][y] = gNum;
  for ( int dx = -NEIGHBOR; dx <= NEIGHBOR; ++dx )
  {
    if ( x + dx < 0 || x + dx >= AREA_WIDTH )
      continue;
    for ( int dy = -NEIGHBOR; dy <= NEIGHBOR; ++dy )
    {
      if ( y + dy < 0 || y + dy >= AREA_HEIGHT )
        continue;
      if ( dx == 0 && dy == 0 )
        continue;
      nMap[x + dx][y + dy]++;
    }
  }
  //printf("AddPoint x:%3d y:%3d\n", x, y);
  return true;
}

bool Liquide::ErasePoint(int x, int y)
{
  if ( bMap[x][y] )
    return false;
  bMap[x][y] = true;
  gMap[x][y] = -1;
  for ( int dx = -NEIGHBOR; dx <= NEIGHBOR; ++dx )
  {
    if ( x + dx < 0 || x + dx >= AREA_WIDTH )
      continue;
    for ( int dy = -NEIGHBOR; dy <= NEIGHBOR; ++dy )
    {
      if ( y + dy < 0 || y + dy >= AREA_HEIGHT )
        continue;
      if ( dx == 0 && dy == 0 )
        continue;
      nMap[x + dx][y + dy]--;
    }
  }
  //printf("ErasePoint x:%3d y:%3d\n", x, y);
  return true;
}

bool Liquide::Swell(int x, int y, Drop &drop)
{
  bool stop = false;
  int sp = 0;
  for ( ; sp < 1000; ++sp)
  {
    for ( int x2 = 0; x2 < AREA_WIDTH; ++x2 )
    {
      if ( abs(x2 - x) > sp + 1 )
        continue;
      for ( int y2 = 0; y2 < AREA_HEIGHT; ++y2 )
      {
        if ( abs(y2 - y) > sp + 1 )
          continue;
        if ( bMap[x2][y2] == drop.eraseDrop )
          continue;
        if ( drop.eraseDrop && gMap[x2][y2] != currentGroup )
          continue;
        float dx = x2 - x;
        float dy = y2 - y;
        float sq = sqrtf( dx*dx + dy*dy );
        if ( sp < sq && sq <= sp + 1 )
        {
          if ( drop.eraseDrop )
            ErasePoint(x2, y2);
          else
          {
            if ( AddPoint(x2, y2) )
              dropCounter++;
          }
          stop = true;
          break;
        }
      }
      if (stop) break;
    }
    if (stop) break;
  }
  if ( sp == 1000 )
  {
    printf("warning Swell: add point not found\n");
    return false;
  }
  return true;
}

bool Liquide::DropSpread(Drop &drop)
{
  if ( !drop.Grow() )
    return false;

  Pos &p = drop.pos;
  for ( int x = 0; x < AREA_WIDTH; ++x )
  {
    if ( abs(p.x - x ) > drop.spread )
      continue;
    for ( int y = 0; y < AREA_HEIGHT; ++y )
    {
      if ( abs(p.y - y) > drop.spread )
        continue;
      float dx = x - p.x;
      float dy = y - p.y;
      float sq = sqrtf( dx*dx + dy*dy );
      if ( drop.spread - 1 < sq && sq <= drop.spread )
      {
        if ( bMap[x][y] == drop.eraseDrop )
          Swell(x, y, drop);
        else
        {
          if ( drop.eraseDrop )
            ErasePoint(x, y);
          else
          {
            if ( AddPoint(x, y) )
              dropCounter++;
          }
        }
      }
    }
  }
  FloodGroup(TEMPORARY_GROUP, drop.pos);
  FloodGroup(currentGroup, drop.pos);
  
  return true;
}

int Liquide::GetLiquideAmount()
{
  int cnt = 0;
  for ( int x = 0; x < AREA_WIDTH; ++x )
    for ( int y = 0; y < AREA_HEIGHT; ++y )
    {
      if ( !bMap[x][y] )
        cnt++;
    }
  return cnt;
}

void Liquide::AddDrop(int x, int y)
{
  if ( notFixed )
    return;
  currentGroup++;
  wchar_t str[128];
  printf( "AddDrop! - group %d\n", currentGroup );
  //OutputDebugString(str);
  
  if ( x < DROP_R ) x = DROP_R;
  if ( y < DROP_R ) y = DROP_R;
  if ( x >= AREA_WIDTH - DROP_R ) x = AREA_WIDTH - DROP_R - 1;
  if ( y >= AREA_HEIGHT - DROP_R ) y = AREA_HEIGHT - DROP_R - 1;
  
  //x = GameBlockPosX( x );
  //y = GameBlockPosY( y );
  
  drops.push_back( Drop( x, y ) );
  if ( AddPoint(x, y) )
    dropCounter++;
  else
    Swell(x, y, drops.back() );
  notFixed = true;
}

void Liquide::SetShrink()
{
  shrinked = false;
  rpt.Reset();
} 

Hole* Liquide::AddHole(int _x, int _y)
{
  _x = GameBlockPosX( _x );
  _y = GameBlockPosY( _y );
  
  Hole h(_x, _y);
  for ( int x = 0; x < AREA_WIDTH; ++x )
    for ( int y = 0; y < AREA_HEIGHT; ++y )
    {
      if ( gMap[x][y] >= 0 && h.IsInHole(x, y, HOLE_R) )
        return NULL;
    }
  
  holes.push_back( h );
  return &holes.back();
}

void Liquide::EraseDrop(int x, int y)
{
  if ( notFixed )
    return;
  if ( bMap[x][y] )
    return;
  /*
  drops.push_back( Drop( x, y ) );
  drops.back().SetErase( true );
  FloodGroup(drops.size()-1, drops.back().pos);
  ErasePoint(x, y);
  */
  Shrink( gMap[x][y] );
  notFixed = true;
}

bool Liquide::IsEdgeOfGroup(int gNum, Pos& pos)
{
  for ( int x = -1; x <= 1; x++ )
    for ( int y = -1; y <= 1; y++ )
    {
      if ( x == 0 && y == 0 )
        continue;
      if ( pos.x + x < 0 || pos.x + x >= AREA_WIDTH || pos.y + y < 0 || pos.y + y >= AREA_HEIGHT )
        continue;
      if ( gMap[pos.x + x][pos.y + y] == gNum )
        return true;
    }
  return false;
}

bool Liquide::IsInnerEdgeOfGroup(int gNum, Pos& pos)
{
  if ( gMap[pos.x][pos.y] != gNum )
    return false;
  int cnt = 0;
  for ( int x = -1; x <= 1; x++ )
    for ( int y = -1; y <= 1; y++ )
    {
      if ( x == 0 && y == 0 )
        continue;
      if ( pos.x + x < 0 || pos.x + x >= AREA_WIDTH || pos.y + y < 0 || pos.y + y >= AREA_HEIGHT )
        continue;
      if ( gMap[pos.x + x][pos.y + y] != gNum )
        cnt++;
    }
  return ( cnt > 2 );
}

bool Liquide::LowPassEdge(int gNum, int count)
{
  int updateCount = 0;

  // less neightbor point <-> more neighbor point:
  int min = 0, max = (NEIGHBOR*2 + 1)*(NEIGHBOR*2 + 1) - 1;
  int fromX, fromY, toX, toY;
  bool fromFound = false;
  bool toFound = false;
  while ( min + 1 < max )
  {
    while( !fromFound )
    {
      for ( int i = 0; i < AREA_WIDTH; i++ )
        for ( int j = 0; j < AREA_HEIGHT; j++ )
        {
          if ( nMap[i][j] == min && !bMap[i][j] && gMap[i][j] == gNum )
          {
            if (( toFound && abs(i - toX) > 1 && abs(j - toY) > 1)  || !toFound ) 
            {
              fromFound = true;
              fromX = i;
              fromY =j;
            }
          }
        }
      if ( fromFound )
        break;
      min++;
      if ( min + 1 >= max )
        break;
    }
    while( !toFound )
    {
      for ( int i = 0; i < AREA_WIDTH; i++ )
        for ( int j = 0; j < AREA_HEIGHT; j++ )
        {
          Pos pos = Pos(i,j);
          if ( nMap[i][j] == max && bMap[i][j] && IsEdgeOfGroup(gNum, pos) )
          {
            if (( fromFound && abs(i - fromX) > 1 && abs(j - fromY) > 1) || !fromFound )
            {
              toFound = true;
              toX = i;
              toY =j;
            }
          }
        }
      if ( toFound )
        break;
      max--;
      if ( min + 1 >= max )
        break;
    }
    if ( fromFound && toFound )
    {
      ErasePoint(fromX, fromY);
      AddPoint(toX, toY, gNum);
      Pos startPos = Pos(toX, toY);
      FloodGroup( TEMPORARY_GROUP, startPos );
      FloodGroup( gNum, startPos );
      if ( ++updateCount == count )
        break;
      toFound = fromFound = false;
      min = 0;
      max = (NEIGHBOR*2 + 1)*(NEIGHBOR*2 + 1) - 1;
    }
  }

  return ( updateCount > 0 );
}

void Liquide::FloodGroup(int groupNum, Pos &startPos)
{
  Pos floodp = startPos;
  int cnt = Flood(groupNum, &floodp, 0);
  //TRACE("FloodGroup:%d\n", cnt);
}

bool Liquide::IsToBeFlood(int gNum, Pos *pos)
{
  if ( gMap[pos->x][pos->y] == gNum )
    return false;
  if ( pos->x < 0 || pos->x >= AREA_WIDTH || pos->y < 0 || pos->y >= AREA_HEIGHT )
    return false;
  if ( bMap[pos->x][pos->y] )
    return false;
  return true;
}

int Liquide::Flood(int gNum, Pos *startPos, int cnt)
{
  vector <Pos> stack;
  stack.push_back( *startPos );
  cnt = 0;
  gMap[startPos->x][startPos->y] = gNum;
  cnt++;
  do {
    Pos pos = stack.back();
    stack.pop_back();
    for ( int x = -1; x <= 1; x++ )
      for ( int y = -1; y <= 1; y++ )
      {
        if ( x == 0 && y == 0 ) continue;
        Pos next = Pos(pos.x + x, pos.y + y);
        if ( IsToBeFlood(gNum, &next) )
        {
          gMap[next.x][next.y] = gNum;
          cnt++;
          stack.push_back(next);
        }
      }

  } while (stack.size() > 0);
  return cnt;
}

void Liquide::ResumeGroup(int gNum)
{
  for ( int i = 0; i < AREA_WIDTH; i++ )
    for ( int j = 0; j < AREA_HEIGHT; j++ )
    {
      if ( gMap[i][j] == gNum )
      {
        Pos startPos = Pos(i,j);
        FloodGroup( TEMPORARY_GROUP, startPos );
        FloodGroup( gNum, startPos );
        return;
      }
    }
}

char* Liquide::GetDataString(char *str)
{
  char ref[40];
  //            0         10        20        30        40        50        60
  //            +----+----+----+----+----+----+----+----+----+----+----+----+---
  strcpy( ref, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/" );
  strcpy( str, "" );
  int cnt = 0;
  bool flag = true;
  for ( int i = 0; i < AREA_WIDTH; i++ )
    for ( int j = 0; j < AREA_HEIGHT; j++ )
    {
      if ( bMap[i][j] == flag ) {
        cnt++;
      }
      else {
         char buf[2] = { ref[cnt], 0 };
         strcat( str, buf );
         flag = !flag;
         cnt = 1;
      }
      if ( cnt == 63 ) {
          strcat( str, "/" );
          cnt = 0;
      }
    }
  if ( cnt > 0 ) {
    char buf[2] = { ref[cnt], 0 };
    strcat( str, buf );
  }
  return str;
}

char* Liquide::GetGroupString(char *str)
{
  char ref[64];
  //            0         10        20        30        40        50        60
  //            +----+----+----+----+----+----+----+----+----+----+----+----+---
  strcpy( ref, "123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/" );
  strcpy( str, "" );
  int flag = true;
  int cnt = 0;
  for ( int i = 0; i < AREA_WIDTH; i++ )
    for ( int j = 0; j < AREA_HEIGHT; j++ )
    {
      if ( bMap[i][j] == flag ) {
        cnt++;
      }
      else {
         if ( gMap[i][j] >= 0 ) {
            char buf[2] = { ref[gMap[i][j] % 60], 0 };
            strcat( str, buf );
         }
         flag = !flag;
         cnt = 1;
      }
    }
  return str;
}

char* Liquide::GetHoleString(char *str)
{
  strcpy( str, "" );
  char buf[128];
  for ( int i = 0; i < holes.size(); i++ ) {
      sprintf( buf, "<hole>%d|%d|%d</hole>", holes[i].pos.x, holes[i].pos.y, holes[i].count );
      strcat( str, buf );
  }
  return str;
}

char* Liquide::GetDropString(char *str)
{
  strcpy( str, "" );
  char buf[128];
  if ( drops.size() > 0 && drops[drops.size() - 1].IsGrowing() ) {
      sprintf( buf, "<drop>%d|%d|%d</drop>", drops[drops.size() - 1].pos.x, drops[drops.size() - 1].pos.y, drops[drops.size() - 1].spread );
      strcat( str, buf );
  }
  return str;
}

///////// DistanceTable ///////////////////

DistanceTable::DistanceTable()
{
  for ( int x = 0; x < 100; x++ )
    for ( int y = 0; y < 100; y++ )
    {
      data[x][y] = (int)sqrtf( x*x + y*y );
    }
}

int DistanceTable::Distance(int x, int y)
{
  if ( x >= 100 || x <= -100 || y >= 100 || y <= -100 )
    return 1000;
  return data[abs(x)][abs(y)];
}

///////// RandomPositionTable ///////////////////

RandomPositionTable::RandomPositionTable()
{
  index = 0;
  for (int x = 0; x < AREA_WIDTH; ++x)
    for (int y = 0; y < AREA_HEIGHT; ++y)
      pos[index++] = Pos(x, y);
      
  for ( int i = 0; i < AREA_WIDTH*AREA_HEIGHT; i++ )
  {
    int indexA = rand() % (AREA_WIDTH*AREA_HEIGHT);
    int indexB = rand() % (AREA_WIDTH*AREA_HEIGHT);
    Pos tmp = pos[indexA];
    pos[indexA] = pos[indexB];
    pos[indexB] = tmp;
  }
  
  index = 0;
}

void RandomPositionTable::Reset()
{
  index = 0;
}

Pos* RandomPositionTable::Get()
{
  if ( index < AREA_WIDTH*AREA_HEIGHT )
    return &pos[index++];
  else
    return NULL;
}

int RandomPositionTable::GetIndex()
{
  return index;
}

//////////////////////////////////////////////////////////

bool Liquide::EdgeSearch(Pos &pos, Pos *next)
{
  const int dx[9] = {-1, -1, -1, 0, 1, 1, 1, 0, -1};
  const int dy[9] = {-1, 0, 1, 1, 1, 0, -1, -1, -1};

  bool prev = true;
  if ( pos.x + dx[0] >= 0 && pos.y + dy[0] >= 0 )
    prev = ( bMap[pos.x + dx[0]][pos.y + dy[0]] );
  for ( int i = 1; i < 9; i++ )
  {
    bool curr = true;
    if ( pos.x + dx[i] >= 0 && pos.x + dx[i] < AREA_WIDTH && pos.y + dy[i] >= 0 && pos.y + dy[i] < AREA_HEIGHT )
      curr = ( bMap[pos.x + dx[i]][pos.y + dy[i]] );
    if ( prev && !curr )
    {
      next->x = pos.x + dx[i];
      next->y = pos.y + dy[i];
      return true;
    }
    prev = curr;
  }
  return false;
}


bool Liquide::Shrink3()
{
  int eraseCnt = 0;
  vector <Pos> remove;
  
  bool shrinkCompleted = false;
  do
  {
    Pos *p = rpt.Get();
    if ( p == NULL )
    {
      if ( shrinkCompleted )
        break;
      shrinkCompleted = true;
      rpt.Reset();
      p = rpt.Get();
    }
    if ( !bMap[p->x][p->y] && IsEdge( p->x, p->y, 6 ) >= 0 )
    {
      remove.push_back( *p );
      eraseCnt++;
    }
  } while ( eraseCnt < 200 );
  
  for ( int i = 0; i < remove.size(); i++ )
    ErasePoint(remove[i].x, remove[i].y);

  //wchar_t str[64];
  //swprintf( str, 64, _T("shrink3 to index %d\n"), rpt.GetIndex() );
  //OutputDebugString(str);
      
  return shrinkCompleted;
}

int Liquide::Shrink2(int gNum)
{
  vector <Pos> remove;
  for ( int x = 0; x < AREA_WIDTH; ++x )
    for ( int y = 0; y < AREA_HEIGHT; ++y )
    {
      if ( gMap[x][y] == gNum )
      {
        if ( gNum == IsEdge( x, y ) )
          remove.push_back( Pos(x, y) );
      }
    }

  for ( int i = 0; i < remove.size(); i++ )
    ErasePoint(remove[i].x, remove[i].y);

  return remove.size();
}

int Liquide::Shrink(int gNum)
{
  Pos cur;
  int gCnt = 0;
  for ( int x = 0; x < AREA_WIDTH; ++x )
    for ( int y = 0; y < AREA_HEIGHT; ++y )
    {
      if ( gMap[x][y] == gNum )
      {
        if ( gCnt == 0 )
        {
          cur.x = x;
          cur.y = y;
        }
        gCnt++;  
      }
    }

  if ( gCnt == 0 )
    return 0;
    
  Pos start = cur;
  vector <Pos> remove;
  remove.push_back(cur);

  Pos next;
  int cnt = 0;
  do {
    if ( !EdgeSearch( cur, &next ) )
      break;
    if ( ++cnt == 1000 )
      break;
    remove.push_back(cur);
    cur = next;
  } while ( next.x != start.x || next.y != start.y );

  for ( int i = 0; i < remove.size(); i++ )
    ErasePoint(remove[i].x, remove[i].y);

  return remove.size();
}

int Liquide::ShrinkIntoHole(int gNum, int holeNum, Pos *startPos, int r)
{
  Pos cur = *startPos;
  Pos start = cur;
  vector <Pos> remove;

  Pos next;
  int cnt = 0;
  do {
    if ( !EdgeSearch( cur, &next ) )
      break;
    if ( ++cnt == 1000 )
      break;
    remove.push_back(cur);
    cur = next;
  } while ( next.x != start.x || next.y != start.y );

  cnt = 0;
  for ( int i = 0; i < remove.size(); i++ )
  {
    if ( !holes[holeNum].IsInHole( remove[i].x, remove[i].y, r ) )
    {
      if ( ErasePoint(remove[i].x, remove[i].y) )
        holes[holeNum].count++;
      cnt++;
    }
  }

  return cnt;
}

int Liquide::FallIntoHole(int gNum)
{
  vector <Pos> fallen;

  int hole = -1;
  for ( int x = 0; x < AREA_WIDTH; ++x )
    for ( int y = 0; y < AREA_HEIGHT; ++y )
    {
      if ( gMap[x][y] == gNum )
      {
        int holeNum = PointInHole(x, y);
        if ( holeNum >= 0 )
        {
          //holes[holeNum].count++;
          fallen.push_back( Pos(x, y) );
          hole = holeNum;
        }
      }
    }
  //bool fallingFinished = false;
  if ( fallen.size() == 0 )
  {
    if ( holeNum == -1 )
      return hole;
    else
    {
      return -1;
      //fallingFinished = true;
      //hole = holeNum;
    }
  }

  //TRACE("FallIntoHole!\n - on hole %d\n", fallen.size());
  
  int fallArea = 7;
  //if ( fallingFinished )
  //  fallArea += 5;
  Pos startEdge;
  bool startEdgeFound = false;
  for ( int x = 0; x < AREA_WIDTH; ++x )
    for ( int y = 0; y < AREA_HEIGHT; ++y )
    {
      if ( gMap[x][y] == gNum )
      { 
        if ( dt.Distance( x - holes[hole].pos.x, y - holes[hole].pos.y ) < fallArea )
        {
          Pos pos = Pos(x, y);
          fallen.push_back( pos );
          if ( !startEdgeFound && IsInnerEdgeOfGroup( gNum, pos ) )
          {
            startEdge = pos;
            startEdgeFound = true;
          }
        }
        if ( dt.Distance( x - holes[hole].pos.x, y - holes[hole].pos.y ) <= fallArea )
        {
          Pos pos = Pos(x, y);
          fallen.push_back( pos );
        }
      }
    }

  //if ( fallingFinished )
  //  return -1;    

  //TRACE(" - fallen total %d\n", fallen.size());

  int fallCnt = 0;
  if ( startEdgeFound )
  {
    fallCnt = ShrinkIntoHole(gNum, hole, &startEdge, fallArea);
    if ( fallCnt == 0 )
    {
      printf(" - no shrink > erase fallen\n");
      for ( int cnt = 0; cnt < fallen.size(); cnt++ )
      {
        if ( ErasePoint( fallen[cnt].x, fallen[cnt].y ) )
          holes[hole].count++;
      }
      fallCnt = fallen.size();
    }
  }
  else
    printf(" - startEdge not found\n");
 
  //TRACE(" - shrink total %d\n", fallCnt);
  return hole;
}

void Liquide::Divisions()
{
  vector <Pos> div;

  for ( int x = 0; x < AREA_WIDTH; ++x )
    for ( int y = 0; y < AREA_HEIGHT; ++y )
    {
      if ( gMap[x][y] == currentGroup )
      {
        Pos pos = Pos(x, y);
        FloodGroup( TEMPORARY_GROUP + div.size(), pos );
        div.push_back(pos);
      }
    }

  for ( int i = 0; i < div.size(); i++ )
  {
    if ( i > 0 )
    {
      dividedGroup.push_back(currentGroup);
      currentGroup++;
      wchar_t str[128];
      printf( "divided - group %d\n", currentGroup );
      //OutputDebugString(str);
    }
    FloodGroup( currentGroup, div[i] );
  }
}

int Liquide::GetDropCountAndReset()
{
  int ret = dropCounter;
  dropCounter = 0;
  return ret;
}

bool Liquide::Update()
{
  if ( !notFixed )
    return false;
  time++;
  if ( DropSpread(drops[drops.size() - 1]) )
    return true;
  Divisions();
  holeNum = FallIntoHole(currentGroup);
  //bool fixing = LowPassEdge(currentGroup, 4);
  bool fixing = false;
  if ( LowPassEdge(currentGroup, 4) )
    fixing = true;
  for ( int i = 0; i < dividedGroup.size(); i++ )
  {
    int holeNumTmp = FallIntoHole(dividedGroup[i]);
    if ( holeNumTmp >= 0 )
      holeNum = holeNumTmp;
    if ( LowPassEdge(dividedGroup[i], 4) )
      fixing = true;
  }
  if ( fixing || holeNum >= 0 )
    return true;
  if ( !shrinked )
  {
    //for ( int i = 0; i <= currentGroup; i++ )
    //  Shrink2( i );
    shrinked = true;
    allGroupsTouched = true;
    Shrink3();
    return true;
  }
  if ( allGroupsTouched )
  {
    for ( int i = 0; i < currentGroup; i++ )
    {
      if ( LowPassEdge(i, 4) )
        fixing = true;
    }
  }
  if ( fixing )
    return true;
  else
    allGroupsTouched = false;
  notFixed = false;
  dividedGroup.clear();
  return true;
}

int Liquide::IsEdge(int posx, int posy, int max)
{
  int cnt = 0;
  int group = -1;
  for ( int x = -1; x <= 1; x++ )
    for ( int y = -1; y <= 1; y++ )
    {
      if ( posx + x < 0 || posx + x >= AREA_WIDTH || posy + y < 0 || posy + y >= AREA_HEIGHT )
        continue;
      if ( !bMap[posx + x][posy + y] )
      {
        cnt++;
        group = gMap[posx + x][posy + y];
      }
    }
  if ( cnt > 0 && cnt < max )
    return group;
  else
    return -1;
}

void Liquide::Reset()
{
  drops.clear();
  holes.clear();
  for ( int i = 0; i < AREA_WIDTH; i++ )
    for ( int j = 0; j < AREA_HEIGHT; j++ )
    {
      bMap[i][j] = true;
      nMap[i][j] = 0;
      gMap[i][j] = -1;
    }
  currentGroup = -1;
  notFixed = false;
  shrinked = true;
  allGroupsTouched = false;
  holeNum = -1;
  dividedGroup.clear();
  dropCounter = 0;
  time = 0;
}

int Liquide::GetTime()
{
  return time;
}

int Liquide::PointInHole(int x, int y, int r)
{
  for ( int i = 0; i < holes.size(); i++ )
  {
    if ( holes[i].IsInHole(x, y, r) )
      return i;
  }
  return -1;
}

void Liquide::DumpNMap()
{
  for ( int y = 0; y < AREA_HEIGHT; ++y )
  {
    wchar_t str[256];
    for ( int x = 0; x < AREA_WIDTH; ++x )
    {
      printf( "%d", nMap[x][y] );
      //OutputDebugString(str);
    }
    printf( "\n" );
    //OutputDebugString(str);
  }
}

int Liquide::GameBlockPosX(int x)
{
  return x / GAME_BLOCK * GAME_BLOCK + GAME_BLOCK / 2;
}

int Liquide::GameBlockPosY(int y)
{
  return y / GAME_BLOCK * GAME_BLOCK + GAME_BLOCK / 2;
}

