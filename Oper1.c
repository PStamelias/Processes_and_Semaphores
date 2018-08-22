/*File Oper1.c*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>    
#include <sys/time.h>
#include <sys/sem.h>
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
};
/* increment semaphore*/
struct sembuf semopinc = {
  .sem_num = 0,
  .sem_op = 1,
  .sem_flg = 0
};    
/* decrement semaphore, may block */
struct sembuf semopdec = {
  .sem_num = 0,
  .sem_op = -1,
  .sem_flg = 0
}; 
void DeleteSemaphoreSet(int id)
{
	if (semctl(id, 0, IPC_RMID, NULL) == -1)
	{
		perror("Error releasing semaphore!");
		exit(EXIT_FAILURE);
	}
}
int main(int argc,char ** argv)
{
	struct timeval  tv1,tv2;
	float Running_Average=0.00;
	int n=atoi(argv[1]);/* Counter of n fork()-children*/
	int parent_id,j,i;
	int m=atoi(argv[2]);/* Size of consumer_table*/
	if(!(m>3000))
		return EXIT_FAILURE;
	union semun arg;
	arg.val=0;/*Beginning value of Semaphores*/
	int *consumer_table,*my_table;
	int sem1=semget((key_t)89999,1,IPC_CREAT | 0660);/*Initialization of Semaphore 1*/
	int sem2=semget((key_t)11111,1,IPC_CREAT | 0660);/*Initialization of Semaphore 2*/
	int sem3=semget((key_t)756565,1,IPC_CREAT | 0660);/*Initialization of Semaphore 3*/
	int sem4=semget((key_t)726565,1,IPC_CREAT | 0660);/*Initialization of Semaphore 4*/
	if (sem1 < 0) {/* Checking Success Initialization*/
    	perror("semget-Sem1");
    	return -1;
	}
	if (sem2 < 0){/* Checking Success Initialization*/
		perror("semget-Sem2");
		return -1;
	}
	if (sem3 < 0){/* Checking Success Initialization*/
		perror("semget-Sem3");
		return -1;
	}
	if (sem4 < 0){/*CHecking Success Initialization*/
		perror("semget-Sem4");
		return -1;
	}
	if(semctl(sem1,0,SETVAL,arg)==-1)/*Setting value of Semaphore 1*/
	{	
		perror("Failed to set value to semaphore1");
		return -1;	
	}
	if(semctl(sem2,0,SETVAL,arg)==-1)/*Setting value of Semaphore 2*/
	{
		perror("Failed to set value to semaphore2");
		return -1;
	}
	if(semctl(sem3,0,SETVAL,arg)==-1)/*Setting value of Semaphore 3*/
	{
		perror("Failed to set value to semaphore3");
		return -1;
	}
	if(semctl(sem4,0,SETVAL,arg)==-1)/*Setting value of Semaphore 4*/
	{
		perror("Failed to set value to semaphore4");
		return -1;
	}
	pid_t pid=fork();
	if( pid > 0 )/*Parent Process*/
	{
		parent_id=getpid();
		my_table=malloc(m*sizeof(int));
		for(i=0;i<m;i++)
			my_table[i]=rand()%100000;
		for(j=0;j<n-1;j++)
		{
			if(getpid()==parent_id)
				fork();/* All Child processes have the same parent process*/
		}
	}
	FILE* f;
	if(getpid()==parent_id)
	{
		time_t timestamp;
		for(j=0;j<m;j++)
		{
			f=fopen("Shared_Memory.txt","w");
			gettimeofday(&tv1, NULL);
			double time_in_mill1 =(tv1.tv_sec) * 1000 + (tv1.tv_usec) / 1000 ; // convert tv_sec & tv_usec to millisecon
			fprintf(f,"%d %f\n",my_table[j],time_in_mill1);/*Parent process write data  to SharedMemory.txt*/
			fclose(f);
			for(i=0;i<n;i++)
				semop(sem1,&semopinc,1);/*Allow Chidren process run*/
			for(i=0;i<n;i++)
				semop(sem2,&semopdec,1);/*Blocking Parent process until all chidren process read data*/
		}
	}
	else
	{
		consumer_table=malloc(m*sizeof(int));
		int e;
		double ty;
		float DT;
		for(j=0;j<m;j++)
		{
			semop(sem1,&semopdec,1);/*Running only when Parent process is blocking*/
			f=fopen("Shared_Memory.txt","r");
			fscanf(f,"%d %lf",&e,&ty);/*Reading data from SharedMemory.txt*/
			consumer_table[j]=e;
			gettimeofday(&tv2,NULL);
			double time_in_mill2 =(tv2.tv_sec) * 1000 + (tv2.tv_usec) / 1000 ;
			DT=time_in_mill2-ty;
			Running_Average=(Running_Average+DT)/(j+1);
			fclose(f);
			semop(sem2,&semopinc,1);/*Time for parent process to re-write*/
		}
	}
	if(getpid()==parent_id)
	{
		for(i=0;i<n;i++)
		{
			semop(sem3,&semopinc,1);/*Running one by one child processes*/
			semop(sem4,&semopdec,1);/*Unblocking only when one child process finish */
		}
	}
	else
	{
		semop(sem3,&semopdec,1);/*Parent process allow child process run*/
		FILE* g;
		g=fopen("LogFile.txt","a");
		for(j=0;j<m;j++)
			fprintf(g,"%d ",consumer_table[j]);
		fprintf(g,"%d %f\n",getpid(),Running_Average);
		printf("PID:%d and Running_Average:%2.10f millisec\n",getpid(),Running_Average);
		fclose(g);
		free(consumer_table);
		semop(sem4,&semopinc,1);/*Child process finished,time for other child process to run*/
	}
	if(getpid()==parent_id)
	{		
		free(my_table);
		DeleteSemaphoreSet(sem1);/*Deleting Semaphore1*/
		DeleteSemaphoreSet(sem2);/*Deleting Semaphore2*/
		DeleteSemaphoreSet(sem3);/*Deleting Semaphore3*/
		DeleteSemaphoreSet(sem4);/*Deleting Semaphore4*/
		wait(NULL);/*Wait until all child processes to finish for finish parent process*/
	}
	return 0;
}  
