#include<stdio.h>
#include <malloc.h>
#include <stdlib.h>

#define ThreadNum       4096

typedef struct node 
{
  unsigned long LockID;
  int	clock[ThreadNum];
  bool isFirst;
  struct node *next;
}*LockLink,LockNode;
typedef struct node1 
{
  unsigned long threadid;
  struct node1 *next;
}*ThreadLink,ThreadNode;
LockLink inilink()
{
  LockLink head;
  head=(LockLink)malloc(sizeof(struct node));
  head->next=NULL;
  for(int i=0;i<ThreadNum;i++)
  {
	head->clock[i] = 0;
  }
  return head;
}
ThreadLink inilink1()
{
  ThreadLink head;
  head = (ThreadLink)malloc(sizeof(struct node1));
  head->next = NULL;
	head->threadid = -1;
  return head;
}