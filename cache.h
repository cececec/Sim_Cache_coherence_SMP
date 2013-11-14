/*******************************************************
                          cache.h
              Amro Awad & Yan Solihin
                           2013
                {ajawad,solihin}@ece.ncsu.edu
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum{
	INVALID = 0,
	//VALID,
	//DIRTY,
	MOD,
	OWN,
	EXC,
	SHD
	//empty
	//valid_exclusive,
	
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq; 
  // ulong states;
   
public:
   cacheLine()				 { tag = 0; Flags = 0; }
   ulong getTag()			 { return tag; }
   ulong getFlags()			 { return Flags;}
   ulong getSeq()			 { return seq; }
   void setSeq(ulong Seq)	 { seq = Seq;}
   void setFlags(ulong flags){ Flags = flags;}
   void setTag(ulong a)		 { tag = a; }
   void invalidate()		 { tag = 0; Flags = INVALID; }//useful function
   bool isValid()			 { return ((Flags) != INVALID);}
  // bool isState(ulong flags) { return ((Flags) == flags);}

  // void setStates(ulong state){  states = state;}
  /* bool ismod()				 { return ((states) == MOD); }
   bool isown()				 { return ((states) == OWN); }
   bool isexc()				 { return ((states) == EXC); }
   bool isshd()				 { return ((states) == SHD); }*/
   //bool isinv()				 { return ((states) == INV); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;
   //******///
   //add coherence counters here///
   //******///
   ulong itoe, itos, mtos, etos, stom, itom, etom, otom, mtoo, stoi, cctransfer, interventions, invalidations, flush;


   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  
     
    Cache(int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM(){return readMisses;} ulong getWM(){return writeMisses;} 
   ulong getReads(){return reads;}ulong getWrites(){return writes;}
   ulong getWB(){return writeBacks;}
   
   void writeBack(ulong)   {writeBacks++;}
   int Access(int,ulong,uchar);
   void printStats(ulong);
   void fprintStats(FILE*,ulong);
   void updateLRU(cacheLine *);

   //******///
   //add other functions to handle bus transactions///
   //******///
   void BusRd(ulong,int);
   void BusRdx(ulong,int);
   void BusUpgr(ulong,int);
   int  HasCopy(ulong,int);
   int  HasCopywrm(ulong,int);//write miss
   void SetShd(ulong,int);
   void SetExc(ulong);
   void SetShdMOESI(ulong,int);	
   void updatec2c(ulong ,int )	;
};

  


#endif
