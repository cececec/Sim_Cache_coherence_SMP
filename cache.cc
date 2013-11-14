/*******************************************************
                          cache.cc
                  Amro Awad & Yan Solihin
                           2013
                {ajawad,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;
 

//s: size a:assoc b:linesize
Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;
   

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));  
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
   itoe=itos=mtos=etos=stom=itom=etom=otom=mtoo=stoi=0; 
   cctransfer=interventions=invalidations=flush=0;
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimensional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**add other parameters to Access()**/
int Cache::Access(int protocol,ulong addr,uchar op)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine *line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;


		if(op == 'w')
		{
			cacheLine *newline = fillLine(addr); 
			newline->setFlags(MOD); 
			itom++; 
			if (protocol==0) return 2;// PrWr/BusRdx
			else if (protocol==1||protocol==2) return 6;
		}//BusRdx--2 
		else 
		{			
			if (protocol==0){ cacheLine *newline = fillLine(addr); newline->setFlags(SHD); itos++;  return 1;}//BusRd--1 
			else if(protocol==1||protocol==2){ return 3; }//search for copy--3
			else { return 0; }
		 }

	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') 
		{
			if (line->getFlags()==EXC)			{etom++;line->setFlags(MOD);return 0;}
			else if (line->getFlags()==OWN)     {otom++;line->setFlags(MOD);}
			else if (line->getFlags()==SHD) 	{stom++;line->setFlags(MOD);}	

			if (protocol==0) return 2;// PrWr/BusRdx
			else if (protocol==1||protocol==2) return 4;// Prwr/  BusUpgr--4

		}
		return 0;//hit--0
	}
	return 0;//hit--0
}

/*look up line*/
cacheLine *Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*allocate a new line @ MISS*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(victim->getFlags() == MOD) { writeBack(addr); }
   else if(victim->getFlags() == OWN) { writeBack(addr); }
   tag = calcTag(addr);   
   victim->setTag(tag);
   //victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine *victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine *Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
  assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}


    //*****************************************//
    //***********Bus transaction***************//
    //*****************************************//



void Cache::BusRd(ulong addr,int protocol)
{
	cacheLine *victim = findLine(addr);
	if (victim!=NULL)
	{
		
		if(victim->getFlags() == OWN)
		{
			flush++;//BusRd/Flush/MOESI		
		}
		
		else if(victim->getFlags() == MOD)
		{
			if(protocol==2)
			{ victim->setFlags(OWN);mtoo++;flush++;}
			else
			{
				victim->setFlags(SHD);
				mtos++;
				interventions++;
				flush++;//BusRd/Flush	
			}
				
		
		}
		else if(victim->getFlags() == EXC)
		{
			victim->setFlags(SHD);
			etos++;
			interventions++;//FlushOpt
		}
	}
}

void Cache::BusRdx(ulong addr,int protocol)
{
	cacheLine *victim = findLine(addr);

    if(victim!=NULL)
	{
		if(victim->getFlags() == OWN)
		{
			victim->setFlags(INVALID);
			invalidations++;
			flush++;//BusRd/Flush/MOESI		
		}
		else if(victim->getFlags() == SHD)
		{
			victim->setFlags(INVALID);
			stoi++;
			invalidations++;
		}
		else if(victim->getFlags() == MOD)
		{
			victim->setFlags(INVALID);
			flush++;
			invalidations++;//BusRdx/Flush
		 }
		else if(victim->getFlags() == EXC)
		{
			victim->setFlags(INVALID);
			invalidations++;
		}
	}
}


void Cache::BusUpgr(ulong addr,int protocol)
{
	cacheLine *victim = findLine(addr);
	if(victim!=NULL)
	{
		if(victim->getFlags() == OWN)
		{
			victim->setFlags(INVALID);
			invalidations++;	
		}
	    else if(victim->getFlags() == SHD)
		{
				victim->setFlags(INVALID);
				stoi++;
				invalidations++;			
		}
	}
}


int Cache::HasCopy(ulong addr,int protocol)
{
	cacheLine *line = findLine(addr);
	
	if(line != NULL)
		{if (protocol==2&&(line->getFlags()==EXC||line->getFlags()==MOD||line->getFlags()==OWN)) 		
			return 2;		
		else 
		return 1;}
	else
		return 0;
}


int Cache::HasCopywrm(ulong addr,int protocol)
{
	cacheLine *line = findLine(addr);
	
	if(line != NULL)
		{if (protocol==2&&(line->getFlags()==EXC)) 		
			return 1;		
		else if	(protocol==1&&(line->getFlags()==EXC||line->getFlags()==SHD)) 	
		return 1;
		else  return 0;}
	else
		return 0;
}


void Cache::updatec2c(ulong addr,int protocol)	
{
	
	if (protocol==1||protocol==2)cctransfer++;
	
}


void Cache::SetShd(ulong addr,int protocol)	
{
	cacheLine *newline = fillLine(addr);
	newline->setFlags(SHD);	
	itos++;
	if (protocol==1||protocol==2)cctransfer++;
	
}

void Cache::SetExc(ulong addr)
{
	cacheLine *newline = fillLine(addr);
	newline->setFlags(EXC);
	itoe++;

}

void Cache::SetShdMOESI(ulong addr,int protocol)	
{
	cacheLine *newline = fillLine(addr);
	newline->setFlags(SHD);	
	itos++;
	
}

void Cache::printStats(ulong i)
{ /****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
	cout << "===== Simulation results (Cache_"<< i <<")      =====" <<endl;
	cout << "01. number of reads:                            "<< reads <<endl;
	cout << "02. number of read misses:                      "<< readMisses <<endl;
	cout << "03. number of writes:                           "<< writes <<endl;
	cout << "04. number of write misses:                     "<< writeMisses <<endl;
	cout << "05. number of write backs:                      "<< writeBacks <<endl;
	cout << "06. number of invalid to exclusive (INV->EXC):  "<< itoe <<endl;
	cout << "07. number of invalid to shared (INV->SHD):     "<< itos <<endl;
	cout << "08. number of modified to shared (MOD->SHD):    "<< mtos <<endl;
	cout << "09. number of exclusive to shared (EXC->SHD):   "<< etos <<endl;
	cout << "10. number of shared to modified (SHD->MOD):    "<< stom <<endl;
	cout << "11. number of invalid to modified (INV->MOD):   "<< itom <<endl;
	cout << "12. number of exclusive to modified (EXC->MOD): "<< etom <<endl;
	cout << "13. number of owned to modified (OWN->MOD):     "<< otom <<endl;
	cout << "14. number of modified to owned (MOD->OWN):     "<< mtoo <<endl;
	cout << "15. number of shared to invalid (SHD->INV):     "<< stoi <<endl;
	//cctransfer: FlushOpt 有问题
	cout << "16. number of cache to cache transfers:         " << cctransfer <<endl;
	//intervntions: Exc/Mod -> Shd   有问题                       
	cout << "17. number of interventions:                    " << interventions <<endl;
	//invaliations: Any -> Inv                                 
	cout << "18. number of invalidations:                    " << invalidations <<endl;
	cout << "19. number of flushes:                          " << flush <<endl;                

}


// void Cache::fprintStats(FILE *out,ulong i)
// {

	// fprintf(out,"===== Simulation results (Cache_%d)      =====\n",(int)i);
	// fprintf(out,"01. number of reads:                            %d\n",(int)reads);
	// fprintf(out,"02. number of read misses:                      %d\n",(int)readMisses);
	// fprintf(out,"03. number of writes:                           %d\n",(int)writes);
	// fprintf(out,"04. number of write misses:                     %d\n",(int)writeMisses);
	// fprintf(out,"05. number of write backs:                      %d\n",(int)writeBacks);
	// fprintf(out,"06. number of invalid to exclusive (INV->EXC):  %d\n",(int)itoe);
	// fprintf(out,"07. number of invalid to shared (INV->SHD):     %d\n",(int)itos);
	// fprintf(out,"08. number of modified to shared (MOD->SHD):    %d\n",(int)mtos);
	// fprintf(out,"09. number of exclusive to shared (EXC->SHD):   %d\n",(int)etos);
	// fprintf(out,"10. number of shared to modified (SHD->MOD):    %d\n",(int)stom);
	// fprintf(out,"11. number of invalid to modified (INV->MOD):   %d\n",(int)itom);
	// fprintf(out,"12. number of exclusive to modified (EXC->MOD): %d\n",(int)etom);
	// fprintf(out,"13. number of owned to modified (OWN->MOD):     %d\n",(int)otom);
	// fprintf(out,"14. number of modified to owned (MOD->OWN):     %d\n",(int)mtoo);
	// fprintf(out,"15. number of shared to invalid (SHD->INV):     %d\n",(int)stoi);
	// //cctransfer: FlushOpt                                             (int)
	// fprintf(out,"16. number of cache to cache transfers:         %d\n",(int)cctransfer);
	// //interventions: Exc/Mod -> Shd                                    (int)
	// fprintf(out,"17. number of interventions:                    %d\n",(int)interventions);
	// //invalidations: Any -> Inv                                        (int)
	// fprintf(out,"18. number of invalidations:                    %d\n",(int)invalidations);
	// fprintf(out,"19. number of flushes:                          %d\n",(int)flush);                      
	

// }
