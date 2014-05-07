#include <stdio.h>
#include "pin.H"
#include "LinkSet.h"
#include "LockSet.h"
#include "ConflictSet.h"
#include <time.h>
#include <iostream>

using namespace std;
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define IsLeaf(s) (((s)->left == NULL) ? True : False)
#define ThreadNum	4096
#define PIN_PRINT(x)	cerr << x << endl;
#ifdef _WIN32
#define PIN_GetLock GetLock
#define PIN_ReleaseLock ReleaseLock
#define PIN_InitLock InitLock
#define MutexInit "CreateMutexA"
#define MutexDestroy "CloseHandle"
#define MutexLock "WaitForSingleObject"/*"EnterCriticalSection"*/
#define MutexUnlock "ReleaseMutex"
#define ThreadCreate "CreateThread"
#define OK 1
#else
#define MutexInit "pthread_mutex_init"
#define MutexDestroy "pthread_mutex_destroy"
#define MutexLock "pthread_mutex_lock"
#define MutexUnlock "pthread_mutex_unlock"
#define OK 0
#endif

PIN_LOCK lock;
static UINT64 mrcount[ThreadNum] = {0};
static UINT64 mwcount[ThreadNum] = {0};
static UINT64 tcount = 0;
static UINT64 curtcount = 0;
static UINT64 LockCount = 0;
static UINT64 UnlockCount = 0;
LinkSet RecordData[ThreadNum];
LockLink LL;
ThreadLink TL;
ConflictLink ConflictSet;
int globalclock[ThreadNum][ThreadNum] = {0};
int threadclock[ThreadNum][ThreadNum] = {0};
FILE *file;
FILE *test;
/*
void PrintSet(Set s, int ident = 0)
{
 
  // printf("\nStart-------------\n");
	if (s != NULL)
	{
		for(int i = 0; i < ident * 2; i++){
			fprintf(test," ");
		}
		fprintf(test,"0x%08x 0x%08x\n", s->front, s->rear);
		if(s->rear < s->front){
				fprintf(test,"->>>>>>>>>>>>\n");
		}
		if (IsLeaf(s)){}
			//fprintf(test,"0x%08x 0x%08x\n", s->front, s->rear);
			//if(s->rear < s->front){
			//	fprintf(test,"->>>>>>>>>>>>\n");
			//}
			//printf("0x%08x-0x%08x ", s->front, s->rear);
		else
		{
			PrintSet(s->left, ident + 1);
			PrintSet(s->right, ident + 1);
		}
	}
//	printf("\nEnd-------------\n");
//	fclose(test);
}*/
void PrintSet(Set s)
{
	if (s != NULL)
	{
		if (IsLeaf(s)){
			fprintf(test,"0x%08x-0x%08x\n", s->front, s->rear);
			if(s->rear < s->front){
				fprintf(test,"->>>>>>>>>>>>\n");
			}
			}
		else
		{
			PrintSet(s->left);
			PrintSet(s->right);
		}
	}
}
void printLock()
{
	fprintf(test,"Lock printf begin\n");
	LockLink p = LL->next;
	while(p != NULL){
		fprintf(test,"lock = %d\n",p->LockID);
		p = p->next;
	}
	fprintf(test,"Lock printf end\n");
}
void PrintThread()
{
	fprintf(test,"current thread link:\n");
	ThreadLink p = TL->next;
	while(p != NULL){
		fprintf(test,"%d ",p->threadid);
		p = p->next;
	}
	fprintf(test,"\n");	
}
// Note that opening a file in a callback is only supported on Linux systems.
// See buffer-win.cpp for how to work around this issue on Windows.
//
// This routine is executed every time a thread is created.


VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
//PIN_PRINT("ThreadStartB");
	PIN_GetLock(&lock, threadid+1);
	tcount++;
	curtcount++;
	ThreadLink q;
  q = (ThreadLink)malloc(sizeof(struct node1));
	q->threadid = (unsigned long)threadid;
	if(TL->next == NULL){
		TL->next = 	q;
		q->next = NULL;
	}else{
		q->next = TL->next;	
		TL->next = q;
	}
	//PrintThread();
	time_t timep;
  struct tm *p;
  time(&timep);
  p = gmtime(&timep); 
  fprintf(test,"At %d:%d:%d Thread id = %d is created.And the Total Thread num = %d\n", p->tm_hour+8,p->tm_min, p->tm_sec,threadid,curtcount);
	// Initialize the threads LinkSet
	
	InitSets(RecordData[threadid]);
	Link NewNode;
	CreateNode(NewNode);
	InsertNode(RecordData[threadid],NewNode);
	//RecordData[threadid].head->next->data.clock[threadid]++;
	//printf("threadid=%d, p=%p\n", threadid, RecordData[threadid].head->next);
	PIN_ReleaseLock(&lock);
//PIN_PRINT("ThreadStartE");
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
//PIN_PRINT("ThreadFiniB");
	PIN_GetLock(&lock, threadid+1);
	curtcount--;
	ThreadLink p1,q;
	p1 = TL;
	q = TL->next;
	while(q != NULL){
		if(q->threadid == (unsigned long)threadid){
			p1->next = q->next;
			free(q);
			break;
		}
		p1 = p1->next;
		q = q->next;
	}
	//PrintThread();
	time_t timep;
  struct tm *p;
  time(&timep);
  p = gmtime(&timep); 
  fprintf(test,"At %d:%d:%d Thread id = %d is exited.And the Total Thread num = %d\n", p->tm_hour+8,p->tm_min, p->tm_sec,threadid,curtcount);
	Link NewNode;
	CreateNode(NewNode);
	InsertNode(RecordData[threadid],NewNode);
	RecordData[threadid].head->next->data.clock[threadid]++;
	PIN_ReleaseLock(&lock);
//PIN_PRINT("ThreadFiniE");
}
VOID BeforeCreateThread(THREADID threadid,ADDRINT retval)
{
	if(retval == 0) {
		return;
	}
	PIN_PRINT("BeforeCreateThreadB");
	PIN_GetLock(&lock, threadid+1);
	Link NewNode;
	CreateNode(NewNode);
	InsertNode(RecordData[threadid],NewNode);
	RecordData[threadid].head->next->data.clock[threadid]++;
	int curid = (int)tcount - 1;
	globalclock[threadid][curid] = RecordData[threadid].head->next->data.clock[threadid];
	threadclock[threadid][curid] = RecordData[threadid].head->next->data.clock[threadid];
	printf("globalclock[%d][%d] = %d\n",threadid,curid,globalclock[threadid][curid]);
	PIN_ReleaseLock(&lock);
	PIN_PRINT("BeforeCreateThreadE");
}
// This routine is executed each time pthread_mutex_init is called.
VOID BeforeInitLock(ADDRINT ReVal, THREADID threadid,ADDRINT getlock)
{
	if(OK) {
		if(!ReVal)  return;
	}else{
	  if(ReVal)   return;
	}
//PIN_PRINT("BeforeInitLockB");
//printf("BeforeInitLock %d\n",getlock);
	PIN_GetLock(&lock, threadid+1);
	LockLink p;
	p = (LockLink)malloc(sizeof(struct node));
	if(OK){
		p->LockID = (unsigned long)ReVal;
	}else{
		p->LockID = (unsigned long)getlock;
	}
	p->isFirst = true;
	if(LL->next == NULL)
	{
		LL->next = p;
		p->next = NULL;
	}	
	else
	{
		p->next = LL->next;
		LL->next = p;
	}
	//printLock();
	PIN_ReleaseLock(&lock);
//PIN_PRINT("BeforeInitLockE");
}

// This routine is executed each time pthread_mutex_destroy is called.
VOID BeforeDestroyLock(ADDRINT getlock, THREADID threadid,ADDRINT ReVal)
{
	if(OK) {
		if(!ReVal)  return;
	}else{
	  if(ReVal)   return;
	}
	PIN_PRINT("BeforeDestroyLockB");
	//printf("BeforeDestroyLock %d\n",getlock);
	PIN_GetLock(&lock, threadid+1);
	//fprintf(test,"CurLock = %d\n",getlock);
	//printLock();
	LockLink p,q;
	p = LL;
	q = LL->next;
	while(q != NULL)
	{
		if(q->LockID == (unsigned long)getlock)
		{
			p->next = q->next;
			free(q);
			break;
		}
		p = p->next;
		q = q->next;
	}
	PIN_ReleaseLock(&lock);
	PIN_PRINT("BeforeDestroyLockE");
}
// This routine is executed each time pthread_mutex_lock is called.
VOID BeforeGetLock( THREADID threadid,ADDRINT getlock ,ADDRINT ReVal)
{
	fprintf(test,"BeforeGetLock, OK=%d, ReVal=%d\n", OK, ReVal);
	if(OK) {
		if(ReVal)  return;
	}else{
	  if(ReVal)   return;
	}
	PIN_PRINT("BeforeGetLockB");
	PIN_GetLock(&lock, threadid+1);
	LockCount++;
	printf("thread id = %d\n",threadid);
	//fprintf(test,"CurLock = %d\n",getlock);
	//printLock();
	//Fix the Lock's location.
	unsigned long CurLockID = getlock;
	LockLink	Lock;
	Lock = LL->next;
	LockLink lckTemp = NULL;

	while(Lock != NULL)
	{
		if(CurLockID == Lock->LockID){
			lckTemp = Lock;
			break;
		}
		else
			Lock = Lock->next;
	}
	//fprintf(test,"Out CurLock = %d\n",getlock);
	//printLock();
	if(lckTemp != NULL){
		fprintf(test,"CurLock = %d\n",getlock);
		printLock();
		if(lckTemp->isFirst){
			lckTemp->isFirst = false;
			fprintf(test,"isFirst = true,threadid = %d,lock = %d\n",threadid,getlock);
		}else{
			fprintf(test,"isFirst = false,threadid = %d,lock = %d\n",threadid,getlock);
			//fprintf(test,"lock Find,lockID = %d,ThreadNum = %d\n",getlock,tcount);
			//Detect conflict
			//int j = (int)tcount;
			//ThreadLink tLink = TL->next;
			//fprintf(file,"\n------------------BeforeGetLock  threadid = %d\n",threadid);
			//for(int k = 0; k < j; k++)
			for(int k = 0;k < (int)tcount;k++)
			{
				fprintf(test,"cur threadid = %d,compare threadid = %d,lock = %d\n",threadid,k,getlock);
				if(k == (int)threadid)
					continue;
				int itsLower = globalclock[k][threadid];
				int itsUpper = Lock->clock[k];
				int myUpper = RecordData[threadid].head->next->data.clock[threadid];
				int myLower = Max(Lock->clock[threadid],threadclock[threadid][k]);
				/*int myLower = globalclock[threadid][k];
				int myUpper = RecordData[threadid].head->next->data.clock[threadid];
				int itsLower = RecordData[threadid].head->next->data.clock[k];
				int itsUpper = Lock->clock[k];*/
				/*int itsLower = 0;
				int itsUpper = RecordData[k].head->next->data.clock[k];
				int myUpper = RecordData[threadid].head->next->data.clock[threadid];
				int myLower = 0;*/
				fprintf(test,"Lock Clock:\n");
				for(int mmm = 0;mmm < (int)tcount;mmm++){
					fprintf(test,"Lock->clock[%d] = %d\n",mmm,Lock->clock[mmm]);	
				}
				fprintf(test,"myLow = %d,myUpper = %d,itsLower = %d,itsUpper = %d,compare id = %d\n",myLower,myUpper,itsLower,itsUpper,k);
				Link myUpperPointer = NULL;
				Link itsUpperPointer = NULL;
				//Current thread's pointer
				myUpperPointer = RecordData[threadid].head->next;
			   //Contrast thread's pointer
				itsUpperPointer = RecordData[k].head->next;
				if(myUpper < myLower || itsUpper <= itsLower){
					continue;
				}
				fprintf(test,"here\n");
				while(itsUpperPointer != NULL  && itsUpperPointer->data.clock[k] >= itsUpper)
					{
						itsUpperPointer = itsUpperPointer->next;
					}
				while(myUpperPointer != NULL && myUpperPointer->data.clock[threadid] > myUpper )
					{
						myUpperPointer = myUpperPointer->next;
					}
				if(itsUpperPointer == NULL || myUpperPointer == NULL)
					continue;
			
			
				Link myTemp = NULL;
				Link itsTemp = NULL;
				//printf("here1\n");
				myTemp = myUpperPointer;
				for(int i = myUpper; i >= myLower ; i--){
					itsTemp = itsUpperPointer;
					for(int j = itsUpper - 1; j >= itsLower ; j--){
						Set RandW = Intersection(itsTemp->data.MemReadSet,myTemp->data.MemWriteSet);
						Set WandR = Intersection(myTemp->data.MemWriteSet,itsTemp->data.MemReadSet);
						Set WandW = Intersection(myTemp->data.MemWriteSet,itsTemp->data.MemWriteSet);
							fprintf(test,"compare is doing\n");
						Set Conflict = Union(RandW,WandR);
						Conflict = Union(Conflict,WandW);
						//printf("here3\n");
						if(Conflict != NULL)
						{
							fprintf(test,"DataRace Find in thid = %d and thid = %d\n",threadid,k);
							PrintSet(Conflict);
							//printf("here4\n");
							/*if(ConflictSet == NULL)
							{
								ConflictSet = (ConflictLink)malloc(sizeof(struct connode));
								ConflictSet->data = CreateSet();
								ConflictSet->next = NULL;
							}
							ConflictLink p,q;
							p = ConflictSet;
							while(p->next != NULL)
							{
								p = p->next;
								if(p->next == NULL) printf("p->next = NULL\n");
							}
												printf("here5\n");
							q = (ConflictLink)malloc(sizeof(struct connode));
							q->data = CreateSet();
							q->LocationCur = NULL;
							q->LocationCon = NULL;
							q->data = Union(q->data,Conflict);
							p->next = q;
							q->next = NULL;*/
						}
						itsTemp = itsTemp->next;
					}				
					myTemp = myTemp->next;
				
				}
			//update globalclock
			globalclock[k][threadid] = Lock->clock[k];	
			}
	    }
	}
	//Creat new node and update clock.
	Link NewNode;
	CreateNode(NewNode);
	InsertNode(RecordData[threadid],NewNode);
	RecordData[threadid].head->next->data.clock[threadid]++;
	PIN_ReleaseLock(&lock);
	PIN_PRINT("BeforeGetLockE");
}

// This routine is executed each time pthread_mutex_unlock is called.
VOID BeforeGetUnlock( THREADID threadid,ADDRINT getlock ,ADDRINT ReVal)
{
PIN_PRINT("BeforeGetUnlockB");
	if(OK) {
		if(!ReVal)  return;
	}else{
	  if(ReVal)   return;
	}
	PIN_GetLock(&lock, threadid+1);
	UnlockCount++;
	//Fix the Lock's location.
	unsigned long CurLockID = (unsigned long)getlock;
	LockLink	Lock;
	Lock = LL->next;
	while(Lock != NULL)
	{
		
		if(CurLockID == Lock->LockID)
		{
			//Creat new node and update clock.
			Link NewNode;
			CreateNode(NewNode);
			InsertNode(RecordData[threadid],NewNode);
			RecordData[threadid].head->next->data.clock[threadid]++;

			//printf("RecordData[threadid].head->next->data.clock[0]=%d\n", RecordData[threadid].head->next->data.clock[0]);
			//Take the maximum
			for(int i=0;i<ThreadNum;i++)
			{
				
				if(RecordData[threadid].head->next->data.clock[i] > Lock->clock[i]){
					Lock->clock[i] = RecordData[threadid].head->next->data.clock[i];
				}
				else{
					RecordData[threadid].head->next->data.clock[i] = Lock->clock[i];
				}
			}
		break;
		}
		else
			Lock = Lock->next;
	}
	PIN_ReleaseLock(&lock);
PIN_PRINT("BeforeGetUnlockE");
}

// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 MemReadSize, THREADID threadid )
{
	mrcount[threadid]++;
	unsigned int Address = (unsigned int)addr;
	RecordData[threadid].head->next->data.MemReadSet = Insert(RecordData[threadid].head->next->data.MemReadSet, Address, MemReadSize);
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 MemWriteSize, THREADID threadid )
{
	mwcount[threadid]++;
	unsigned int Address = (unsigned int)addr;
	RecordData[threadid].head->next->data.MemWriteSet = Insert(RecordData[threadid].head->next->data.MemWriteSet, Address, MemWriteSize);
}

//====================================================================
// Instrumentation Routines
//====================================================================

// This routine is executed for each image.
VOID ImageLoad(IMG img, VOID *v)
{
	PIN_PRINT("ImageLoad0");
	RTN rtn0 = RTN_FindByName(img, MutexInit);
	if (RTN_Valid( rtn0 ))
	{
		//fprintf(test,"img = %s,rtn = %s\n",IMG_Name(img).c_str(),RTN_Name(rtn0).c_str());
		//fprintf(test,"rtn0 find\n");
		RTN_Open(rtn0);
		/*if(OK) 	*/	RTN_InsertCall(rtn0, IPOINT_AFTER, AFUNPTR(BeforeInitLock),
					   IARG_FUNCRET_EXITPOINT_VALUE ,
					   IARG_THREAD_ID,
					   IARG_FUNCARG_ENTRYPOINT_VALUE,0,
					   IARG_END);
		/*else RTN_InsertCall(rtn0, IPOINT_BEFORE, AFUNPTR(BeforeInitLock),
					   IARG_FUNCARG_ENTRYPOINT_VALUE,0,
					   IARG_THREAD_ID, IARG_END);*/
				

					   			   
		RTN_Close(rtn0);
	}
PIN_PRINT("ImageLoad1");	
	RTN rtn1 = RTN_FindByName(img, MutexLock);

	if (RTN_Valid( rtn1 ))
	{
		//fprintf(test,"rtn1 find\n");
		RTN_Open(rtn1);
        
		/*RTN_InsertCall(rtn1, IPOINT_BEFORE, AFUNPTR(BeforeGetLock),
					   IARG_THREAD_ID, 
					   IARG_FUNCARG_ENTRYPOINT_VALUE,0,
					   IARG_END);*/
		RTN_InsertCall(rtn1, IPOINT_AFTER, AFUNPTR(BeforeGetLock),
					   IARG_THREAD_ID,
					   IARG_FUNCARG_ENTRYPOINT_VALUE,0,
					   IARG_FUNCRET_EXITPOINT_VALUE ,IARG_END);			   
		//fprintf(test,"MutexLock found!\n");	
		RTN_Close(rtn1);
	}
//	else{
	//	fprintf(test,"MutexLock no found!\n");	
	//}
	
	
PIN_PRINT("ImageLoad2");	
	RTN rtn2 = RTN_FindByName(img, MutexUnlock);

	if (RTN_Valid( rtn2 ))
	{
		//fprintf(test,"rtn2 find\n");
		RTN_Open(rtn2);
/*
		RTN_InsertCall(rtn2, IPOINT_BEFORE, AFUNPTR(BeforeGetUnlock),
					   IARG_THREAD_ID, 
					   IARG_FUNCARG_ENTRYPOINT_VALUE,0,
					   IARG_END);
*/
		RTN_InsertCall(rtn2, IPOINT_AFTER, AFUNPTR(BeforeGetUnlock),
					   IARG_THREAD_ID,
					   IARG_FUNCARG_ENTRYPOINT_VALUE,0,
					   IARG_FUNCRET_EXITPOINT_VALUE ,IARG_END);			   

		RTN_Close(rtn2);
	}
PIN_PRINT("ImageLoad3");
	RTN rtn3 = RTN_FindByName(img, MutexDestroy);
	if(RTN_Valid( rtn3 ))
	{
		//fprintf(test,"rtn3 find\n");
		RTN_Open(rtn3);
/*
		RTN_InsertCall(rtn3, IPOINT_BEFORE, AFUNPTR(BeforeDestroyLock),
					   IARG_FUNCARG_ENTRYPOINT_VALUE,0,
					   IARG_THREAD_ID, IARG_END);
*/
		RTN_InsertCall(rtn3, IPOINT_AFTER, AFUNPTR(BeforeDestroyLock),
					   IARG_FUNCARG_ENTRYPOINT_VALUE,0,
					   IARG_THREAD_ID,
					   IARG_FUNCRET_EXITPOINT_VALUE ,IARG_END);	
		RTN_Close(rtn3);
	}
	RTN rtn4 = RTN_FindByName(img,ThreadCreate);
	if (RTN_Valid(rtn4))
	{
		//fprintf(test,"rtn4 find\n");
		RTN_Open(rtn4);
        
		RTN_InsertCall(rtn4, IPOINT_AFTER, AFUNPTR(BeforeCreateThread),
					   IARG_THREAD_ID,
					   IARG_FUNCRET_EXITPOINT_VALUE ,IARG_END);			   

		RTN_Close(rtn4);
	}
	/*RTN rtn5 = RTN_FindByName(img, "CreateEventW");
	if (RTN_Valid( rtn5))
	{
		//fprintf(test,"img = %s,rtn = %s\n",IMG_Name(img).c_str(),RTN_Name(rtn0).c_str());
		fprintf(test,"CreateEventW find\n");
		RTN_Open(rtn5);
		RTN_InsertCall(rtn5, IPOINT_AFTER, AFUNPTR(BeforeInitLock),
					   IARG_FUNCRET_EXITPOINT_VALUE ,
					   IARG_THREAD_ID,
					   IARG_FUNCARG_ENTRYPOINT_VALUE,0,
					   IARG_END);	   			   
		RTN_Close(rtn5);
	}*/
//PIN_PRINT("ImageLoad");
}

VOID Instruction(INS ins, VOID *v)
{
	// Instruments memory accesses using a predicated call, i.e.
	// the instrumentation is called iff the instruction will actually be executed.
	//
	// On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
	// prefixed instructions appear as predicated instructions in Pin.
	if(!INS_Valid(ins)) return;
	UINT32 memOperands = INS_MemoryOperandCount(ins);
	// Iterate over each memory operand of the instruction.
	for (UINT32 memOp = 0; memOp < memOperands; memOp++)
	{
		if (INS_MemoryOperandIsRead(ins, memOp))
		{
			UINT32 MemReadSize = INS_MemoryReadSize(ins);
			if(!INS_IsStackRead	(ins)){
				INS_InsertPredicatedCall(
					ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
					IARG_INST_PTR,
					IARG_MEMORYOP_EA, memOp,
					IARG_UINT32, MemReadSize,
					IARG_THREAD_ID,
					IARG_END);
				}//else printf("stack read\n");
		}
		// Note that in some architectures a single memory operand can be 
		// both read and written (for instance incl (%eax) on IA-32)
		// In that case we instrument it once for read and once for write.
		if (INS_MemoryOperandIsWritten(ins, memOp))
		{
			UINT32 MemWriteSize = INS_MemoryWriteSize(ins);
			//printf("writesize %d\n",MemWriteSize);
			if(!INS_IsStackWrite(ins)){
				INS_InsertPredicatedCall(
					ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
					IARG_INST_PTR,
					IARG_MEMORYOP_EA, memOp,
					IARG_UINT32, MemWriteSize,
					IARG_THREAD_ID,
					IARG_END);
				}//else { printf("stack write\n");}
		}
	}
}

// This routine is executed once at the end.
VOID Fini(INT32 code, VOID *v)
{
	int tmp1 = 0;
	int tmp2 = 0;
	fprintf(test,"every thread read and write is:\n");
	for(int i = 0;i < (int)tcount;i++)
	{
		tmp1 += mrcount[i];
		tmp2 += mwcount[i];
		fprintf(test,"mrcount[%d] = %d,mwcount[%d] = %d\n",i,(int)mrcount[i],i,(int)mwcount[i]);
	}
	//printf("tcount = %lu,mrcount = %lu,mwcount = %lu,total = %lu\n",tcount,tmp1,tmp2,(tmp1+tmp2));
	fprintf(test,"total read and write.mrcount = %d,mwcount = %d,total = %d\n",tmp1,tmp2,tmp1+tmp2);
	fprintf(test,"Lock num = %d,Unlock num = %d\n",(int)LockCount,(int)UnlockCount);
	fclose(file);
	fclose(test);
	//PIN_PRINT("end program");
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
	PIN_ERROR("This Pintool prints a trace of malloc calls in the guest application\n"+ KNOB_BASE::StringKnobSummary() + "\n");
	return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(INT32 argc, CHAR **argv)
{
//PIN_PRINT("main");
   file = fopen("race.log","wb");
   test = fopen("test.log","wb");
   if(file == NULL) fprintf(file,"打开文件失败\n");
   if(test == NULL) fprintf(test,"打开文件失败\n");
   fprintf(file,"进入文件race\n");
   fprintf(test,"进入文件test\n");
	// Initialize the pin lock
	PIN_InitLock(&lock);

	// Initialize the LockLink	
	LL = inilink();
	TL = inilink1();
	// Initialize pin
	if (PIN_Init(argc, argv)) return Usage();
	PIN_InitSymbols();

	// Register ImageLoad to be called when each image is loaded.
	IMG_AddInstrumentFunction(ImageLoad, 0);

//PIN_PRINT("main");
	//
	INS_AddInstrumentFunction(Instruction, 0);

//PIN_PRINT("main");
	// Register Analysis routines to be called when a thread begins/ends
	PIN_AddThreadStartFunction(ThreadStart, 0);
//PIN_PRINT("main");
	PIN_AddThreadFiniFunction(ThreadFini, 0);

//PIN_PRINT("main");
	// Register Fini to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);

//PIN_PRINT("main");
	// Never returns
	PIN_StartProgram();
	return 0;
}
