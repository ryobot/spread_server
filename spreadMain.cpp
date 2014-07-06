#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "Liquide.h"

#define BUF_SIZE 50

char wbuf[32768];
char message[32768];
//char databuf[2048];

Liquide lq;
int cnt;

typedef struct {
  int sock;
  struct sockaddr_in saddr;
} clientdata;

typedef struct {
  unsigned int timestamp;
  char data[1024];
  char group[1024];
} spread_data;

typedef struct {
  spread_data data[BUF_SIZE];
  int cursor;
  unsigned int currentTime;
  void init() {
    lq.Reset();
    cursor = 0;
    currentTime = 0;
    lq.GetDataString(data[cursor].data);
    lq.GetGroupString(data[cursor].group);
    data[cursor].timestamp = currentTime++;
    cursor++;
  }
  bool update() {
    if ( lq.Update() ) {
      lq.GetDataString(data[cursor].data);
      lq.GetGroupString(data[cursor].group);
      data[cursor].timestamp = currentTime++;
      if ( ++cursor == BUF_SIZE ) {
        cursor = 0;
      }
      return true;
    }
    return false;
  }
  int getData(unsigned int lastTime, char *buf) {
    char tmp[64];
    strcpy( buf, "<spread_data>" );
    int current = cursor;
    int offset = currentTime - lastTime - 1;
    if ( offset > BUF_SIZE - 1 || lastTime > currentTime ) {
      offset = BUF_SIZE - 1;
      strcat( buf, "<continue>no</continue>" );
    } else {
      strcat( buf, "<continue>yes</continue>" );
    }
    sprintf( tmp, "<numdata>%d</numdata>", offset );
    strcat( buf, tmp );
    for ( int i = offset; i > 0; i-- ) {
      int dataPos = current - i;
      if ( dataPos < 0 ) {
        dataPos += BUF_SIZE;
      }
      sprintf( tmp, "<data><timestamp>%d</timestamp><map>", data[dataPos].timestamp );
      strcat( buf, tmp );
      strcat( buf, data[dataPos].data );
      strcat( buf, "</map><group>" );
      strcat( buf, data[dataPos].group );
      strcat( buf, "</group></data>" );
    }
    strcat( buf, "</spread_data>" );
    return offset;
  }
} spread_data_buffer;

spread_data_buffer spBuf;

enum {
  REQ_INVALID,
  REQ_CONTINUE,
  REQ_JOIN,
  REQ_DROP,
  REQ_RESET
};

typedef struct {
  int request;
  unsigned int lastTime;
  int posX;
  int posY;
} spread_request;

void *threadfunc2(void *data)
{
  while (1) {
    usleep(100000);
    if ( cnt%30 == 0 ) {
      int rx = rand() % 13;
      int ry = rand() % 8;
      lq.AddDrop(20 + rx*10, 20 + ry*10);
    }
    if ( cnt++ == 1999 ) {
      cnt = 0;
      lq.Reset();
    }
    spBuf.update();

    /*
    sprintf(wbuf, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", strlen(message));
    strcat(wbuf, message);
    strcat(wbuf, "\r\n");
    */
  }
}

int getRequest(char *buf, spread_request *request) {
  request->request = REQ_INVALID;
  char all[1024], *head, *body, *tail, *key, *value;

  strcpy( all, buf );
  tail = all;
  head = strtok( tail, "?" );
  body = strtok( NULL, " " );
  
  key = strtok( body, "&=" );
  value = strtok( NULL, "&=" );
  while ( key && value ){
    if ( strcmp( key, "req" ) == 0 ) {
      if ( strcmp( value, "continue" ) == 0 ) {
        request->request = REQ_CONTINUE;
      } else if ( strcmp( value, "drop" ) == 0 ) {
        request->request = REQ_DROP;
      } else if ( strcmp( value, "reset" ) == 0 ) {
        request->request = REQ_RESET;
      } else if ( strcmp( value, "join" ) == 0 ) {
        request->request = REQ_JOIN;
      } else {
        request->request = REQ_INVALID;
      }
    } else if ( strcmp( key, "lasttime" ) == 0 ) {
      request->lastTime = (unsigned int)atoi( value );
    } else if ( strcmp( key, "posx" ) == 0 ) {
      request->posX = (unsigned int)atoi( value );
    } else if ( strcmp( key, "posy" ) == 0 ) {
      request->posY = (unsigned int)atoi( value );
    }
    key = strtok( NULL, "&=" );
    value = strtok( NULL, "&=" );
  }
}

void *threadfunc(void *data)
{
  int sock;
  clientdata *cdata = (clientdata*)data;
  char buf[1024];
  int n;
  spread_request req;

  if (data == NULL) {
    return (void *)-1;
  }

  /* (5) */
  sock = cdata->sock;

  /* (6) */
  n = read(sock, buf, sizeof(buf));
  if (n < 0) {
    perror("read");
    goto err;
  }
  getRequest( buf, &req );
  //printf("request:%s", buf);
  printf("request.request:%d\n", req.request);
  printf("request.lasttime:%d\n", req.lastTime);
  printf("request.posX:%d\n", req.posX);
  printf("request.posY:%d\n", req.posY);

  spBuf.getData( req.lastTime, message );
  sprintf(wbuf, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", strlen(message));
  strcat(wbuf, message);
  strcat(wbuf, "\r\n");
    
  n = write(sock, wbuf, (int)strlen(wbuf));
  if (n < 0) {
    perror("write");
    goto err;
  }

  /* (7) */
  /* TCP繧ｻ繝・す繝ｧ繝ｳ縺ｮ邨ゆｺ・*/
  if (close(sock) != 0) {
    perror("close");
    goto err;
  }

  free(data);
  
  return NULL;

err:
  free(data);
  return (void *)-1;
}

int main()
{
  int sock0;
  struct sockaddr_in addr;
  socklen_t len;
  pthread_t th, th2;
  clientdata *cdata;

  spBuf.init();

  sprintf(message, "#");
  sprintf(wbuf, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", strlen(message));
  strcat(wbuf, message);
  strcat(wbuf, "\r\n");

  if (pthread_create(&th2, NULL, threadfunc2, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }

  /* (1) */
  /* 繧ｽ繧ｱ繝・ヨ縺ｮ菴懈・ */
  sock0 = socket(AF_INET, SOCK_STREAM, 0);

  /* 繧ｽ繧ｱ繝・ヨ縺ｮ險ｭ螳・*/
  addr.sin_family = AF_INET;
  addr.sin_port = htons(12345);
  addr.sin_addr.s_addr = INADDR_ANY;

  bind(sock0, (struct sockaddr *)&addr, sizeof(addr));

  /* TCP繧ｯ繝ｩ繧､繧｢繝ｳ繝医°繧峨・謗･邯夊ｦ∵ｱゅｒ蠕・※繧狗憾諷九↓縺吶ｋ */
  listen(sock0, 5);
  /* (1) 邨ゅｏ繧・*/

  /* (2) */
  for (;;) {
    cdata = (clientdata*)malloc(sizeof(clientdata));
    if (cdata == NULL) {
      perror("malloc");
      return 1;
    }

    /* TCP繧ｯ繝ｩ繧､繧｢繝ｳ繝医°繧峨・謗･邯夊ｦ∵ｱゅｒ蜿励￠莉倥￠繧・*/
    len = sizeof(cdata->saddr);
    cdata->sock = accept(sock0, (struct sockaddr *)&cdata->saddr, &len);

    /* (3) */
    if (pthread_create(&th, NULL, threadfunc, cdata) != 0) {
      perror("pthread_create");
      return 1;
    }

    /* (4) */
    if (pthread_detach(th) != 0) {
      perror("pthread_detach");
      return 1;
    }
  }

  /* (8) */
  /* listen 縺吶ｋsocket縺ｮ邨ゆｺ・*/
  if (close(sock0) != 0) {
    perror("close");
    return 1;
  }

  if (pthread_detach(th2) != 0) {
    perror("pthread_detach");
    return 1;
  }

  return 0;
}

