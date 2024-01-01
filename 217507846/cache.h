#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for L2 and victim caches
	int data; // the actual data stored in the cache/memory
	bool valid;
	// add more things here if needed
};

struct Stat
{
	long double missL1;
	long double missL2;
	long double accL1;
	long double accL2;
	long double accVic;
	long double missVic;
};

class cache {
private:
	cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row 
	cacheBlock victimCache[VICTIM_SIZE]; //4-entry victim cache
	Stat myStat;
public:
	cache();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
	void load(int* data, int adr, int* myMem);
	int loadSearchL1(int adr);
	int loadSearchVictim(int adr);
	int loadSearchL2(int adr);
	int computeL1index(int adr);
	int computeL1tag(int adr);
	int computeVictimTag(int adr);
	int computeL2index(int adr);
	int computeL2tag(int adr);
	void evictL2(int *data, int adr);
	long double getL1MissRate();
	long double getL2MissRate(); 
	long double getVictimMissRate();

	void store(int* data, int adr, int* myMem);
	bool storeSearchL1(int* data, int adr, int* myMem);
	bool storeSearchVictim(int* data, int adr, int* myMem);
	bool storeSearchL2(int* data, int adr, int* myMem);
	void output(int* data, int adr, int* myMem);

};


