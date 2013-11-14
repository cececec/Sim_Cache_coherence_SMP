/*******************************************************
                          main.cc
                  Amro Awad & Yan Solihin
                           2013
                {ajawad,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#define pr //fpr
#define arg
#include "cache.h"

int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;
	int ca=0,i=0;
	int proc_id;
	char op;
	long origadd;
	

#ifdef arg	
	char *fname =  (char *)malloc(20);
	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:MOESI*/
	fname = argv[6];

#endif
#ifdef argn		
	//*********************************************//
    //*****´ýÉ¾¡ª¡ªDebugging only here**********//
	//*********************************************//
	int cache_size = 32678;
	int cache_assoc= 8;
	int blk_size   = 64;
	int num_processors = 16;/*1, 2, 4, 8*/
	int protocol   = 0;	 /*0:MSI, 1:MESI, 2:MOESI*/
	//fname="CGad";//"test.txt";//CGaw
#endif
	//**************************************************//
	//*********print out simulator configuration here*****//	
	//****************************************************//
	#ifdef fpr
		FILE *out;
		out=fopen("out_100.txt", "w");
		if(!out) {printf("cannot open this out file\n");exit(0);};
		fprintf(out,"===== 506 SMP Simulator Configuration =====\n");
		fprintf(out,"L1_SIZE:                        %d\n", cache_size);
		fprintf(out,"L1_ASSOC:                       %d\n", cache_assoc);
		fprintf(out,"L1_BLOCKSIZE:                   %d\n", blk_size);
		fprintf(out,"NUMBER OF PROCESSORS:           %d\n", num_processors);
		fprintf(out,"COHERENCE PROTOCOL:             %d\n", protocol);
		fprintf(out,"TRACE FILE:                     %s\n", fname );
	#endif
	#ifdef pr
		printf("===== 506 SMP Simulator Configuration =====\n");
		printf("L1_SIZE:                        %d\n", cache_size);
		printf("L1_ASSOC:                       %d\n", cache_assoc);
		printf("L1_BLOCKSIZE:                   %d\n", blk_size);
		printf("NUMBER OF PROCESSORS:           %d\n", num_processors);
		if(protocol==0)
		printf("COHERENCE PROTOCOL:             MSI\n");
		else if(protocol==1)
		printf("COHERENCE PROTOCOL:             MESI\n");
		else if(protocol==2)
		printf("COHERENCE PROTOCOL:             MOESI\n");
		printf("TRACE FILE:                     %s\n", fname );
	#endif

	
	//*********************************************//
    //*****create an array of caches here**********//
	//*********************************************//	
	
	Cache **cachesArray;
	cachesArray=(struct Cache **)malloc(num_processors*sizeof(struct Cache));
	
	for(i=0;i<num_processors;i++)
	{
		cachesArray[i]=new Cache(cache_size,cache_assoc,blk_size);
	}

	pFile = fopen (fname,"r");//fname
	if(pFile == 0)
	{   
		printf("Trace file problem\n");
		exit(0);
	}


	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	//******************************************************************//

	int hascopy,hasshd;

	while(!feof(pFile))
	{
			fscanf(pFile,"%d %c %lx\n",&proc_id,&op,&origadd);
			//protocol 0--MSI  1--MESI  2--MOESI
		
			ca=cachesArray[proc_id]->Access(protocol,origadd,op);//0--hit;1--BusRd;2--BusRdx;3--Search;4--BusUpg;5--BusUpdate
			
    //*****************************************//
    //****************Proc Init****************//
    //*****************************************//

		if (protocol==1||protocol==2){
			if(ca==3)  //MESI search copy
				{
					hascopy=0;
					hasshd=0;
					for(i=0;i<num_processors;i++)
					{		
						if(i!=proc_id)
							{if (cachesArray[i]->HasCopy(origadd,protocol)==2) { hasshd=1;hascopy=hascopy+1;}
							else if(cachesArray[i]->HasCopy(origadd,protocol)==1) hascopy=hascopy+1;}
					}
				
					if (hascopy==0)			
						 cachesArray[proc_id]->SetExc(origadd);
					else if (hasshd==0&&protocol==2) 
						 cachesArray[proc_id]->SetShdMOESI(origadd,protocol);		  
					else cachesArray[proc_id]->SetShd(origadd,protocol);
				}
				
			else if (ca==6)//wr miss. if find copy, then cctrans++
			{
				hascopy=0;
			
					for(i=0;i<num_processors;i++)
					{		
						if(i!=proc_id)
							{
								if (cachesArray[i]->HasCopywrm(origadd,protocol)==1) { hascopy=1;}
							}
					}
					
					if (hascopy==1)			
						 cachesArray[proc_id]->updatec2c(origadd,protocol);
		
			}
		}	

    //*****************************************//
    //***********Bus transaction***************//
    //*****************************************//			
			
			for(i=0;i<num_processors;i++)
			{			
				if(i!=proc_id){
			        //***********************MSI***************************//
					if(protocol==0){
						if (ca==2)	cachesArray[i]->BusRdx(origadd,protocol);
						else if (ca==1) cachesArray[i]->BusRd(origadd,protocol);	
					}
					//***********************MESI***************************//
					else {
						if (ca==4)		cachesArray[i]->BusUpgr(origadd,protocol);	
						else if (ca==6) cachesArray[i]->BusRdx(origadd,protocol);
						else if (ca==1||ca==3) cachesArray[i]->BusRd(origadd,protocol);
					}//0--hit;1--BusRd;2--BusRdx;3--Search;4--BusUpg;5--BusUpdate
					
				}
			}
			

	}



	//********************************//
	//print out all caches' statistics //
	//********************************//
	//num_processors
	for(i=0;i<num_processors;i++)
	{
		#ifdef fpr
			cachesArray[i]->fprintStats(out,i);
		#endif

		#ifdef pr
			cachesArray[i]->printStats(i);
		#endif
	}


	//ca=cache_size+cache_assoc+blk_size+num_processors+protocol;
	//printf("Done!\n");
	#ifdef fpr
		fclose(pFile);
		fclose(out);
	#endif
	free(cachesArray);
	//getchar();
	
}
