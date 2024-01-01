#include "cache.h"
#include <iostream>

cache::cache()
{
	// Initializing L1, L2 and victim caches
	for (int i = 0; i < L1_CACHE_SETS; i++)
		L1[i].valid = false;

	for (int i = 0; i < L2_CACHE_SETS; i++){
		for (int j = 0; j < L2_CACHE_WAYS; j++){
			L2[i][j].valid = false;
			L2[i][j].lru_position = j;
		}
	}	

	for (int i = 0; i < VICTIM_SIZE; i++)
	{
		victimCache[i].valid = false;
		victimCache[i].lru_position = i;
	}

	// Miss and access counts for L1, L2 and victim caches
	this->myStat.missL1 = 0;
	this->myStat.missL2 = 0;
	this->myStat.missVic = 0;
	this->myStat.accL1 = 0;
	this->myStat.accL2 = 0;
	this->myStat.accVic = 0;
}

//Returns index of cache line with matching tag if found, otherwise returns -1
int cache::loadSearchL1(int adr){
	//2 bits for block offset, 4 bits for index, and 26 bits for tag
	int blockOffset = adr % 4;
	adr = adr >> 2;
	int index = adr % 16;
	adr = adr >> 4;
	int tag = adr % (1 << 26);
	if (L1[index].valid && L1[index].tag == tag){
		return index;
	}

	return -1;
}

//Computes index in L1 cache given address
int cache::computeL1index(int adr){
	return (adr >> 2) % 16;
}

//Computes tag in L1 cache given address
int cache::computeL1tag(int adr){
	adr = adr >> 6;
	return adr % (1 << 26);
}

//Compute L1 miss rate
long double cache::getL1MissRate(){
	return ((long double) myStat.missL1)/myStat.accL1;
}

//Compute L2 miss rate
long double cache::getL2MissRate(){
	return ((long double) myStat.missL2)/myStat.accL2;
}

//Compute Victim miss rate
long double cache::getVictimMissRate(){
	return ((long double) myStat.missVic)/myStat.accVic;
}

//Computes tag in victim cache given address
int cache::computeVictimTag(int adr){
	adr = adr >> 2;
	return adr % (1 << 30);
}

//Returns index of cache line with matching tag if found, otherwise returns -1
int cache::loadSearchVictim(int adr){
	//2 bits for block offset and 30 bits for tag
	int blockOffset = adr % 4;
	adr = adr >> 2;
	int tag = adr % (1<<30);
	for(int i=0; i<VICTIM_SIZE; i++){
		if(victimCache[i].valid && victimCache[i].tag == tag){
			return i;
		}
	}

	return -1;
}

//Returns way number with matching tag if found, otherwise returns -1
int cache::loadSearchL2(int adr){

	for(int i=0; i<L2_CACHE_WAYS; i++){
		if(L2[computeL2index(adr)][i].valid && L2[computeL2index(adr)][i].tag == computeL2tag(adr)){
			return i;
		}
	}

	return -1;
}

int cache::computeL2index(int adr){
	return (adr >> 2) % (1 << 4);
}

int cache::computeL2tag(int adr){
	return (adr >> 6) % (1 << 26);
}

void cache::evictL2(int *data, int adr){
	int lruBlock;

	//Find least recently used block in set and update contents
	for(int i=0; i<L2_CACHE_WAYS; i++){
		if(L2[computeL2index(adr)][i].lru_position == 0){
			lruBlock = i;
			L2[computeL2index(adr)][lruBlock].lru_position = 7;
			L2[computeL2index(adr)][lruBlock].tag = computeL2tag(adr);
			L2[computeL2index(adr)][lruBlock].data = *data;
			L2[computeL2index(adr)][lruBlock].valid = true;
		}
	}

	//Decrement LRU position of every other block in set
	for(int i=0; i<L2_CACHE_WAYS; i++){
		if(i!=lruBlock){
			L2[computeL2index(adr)][i].lru_position--;
		}
	}
}

void cache::load(int *data, int adr, int *myMem)
{
	int originalAdr = adr;	
	int originalVictimData;
	int originalL1Data;
	int originalL2Data;
	int i = loadSearchL1(originalAdr);
	myStat.accL1++;
	//Match in L1 cache, quit
	if(i >= 0){
		return;
	}

	myStat.missL1++; //L1 miss

	myStat.accVic++;
	int j = loadSearchVictim(originalAdr);
	
	//Found in victim cache!
	if(j >= 0){
		//Bring evicted line from L1 to victim cache (swap)
		originalVictimData = victimCache[j].data;
		int originalL1tag = L1[computeL1index(originalAdr)].tag;
		int originalVictimTag = victimCache[j].tag;
		victimCache[j].tag = (originalL1tag << 4) + computeL1index(originalAdr);
		victimCache[j].data = L1[computeL1index(originalAdr)].data;
		victimCache[j].valid = true;

		//Update victim cache LRU
		int originalVictimLRU = victimCache[j].lru_position;
		for(int i=0; i<VICTIM_SIZE; i++){
			if(victimCache[i].lru_position > originalVictimLRU){
				victimCache[i].lru_position--;
			}
		}
		victimCache[j].lru_position = 3;

		//Bring desired victim cache line to L1
		L1[computeL1index(originalAdr)].data = originalVictimData;
		L1[computeL1index(originalAdr)].tag = (originalVictimTag >> 4) % (1 << 26);
		L1[computeL1index(originalAdr)].valid = true;
		return;
	}

	myStat.missVic++; //Victim cache miss

	myStat.accL2++;

	int k = loadSearchL2(originalAdr);
	//Found in L2
	if(k>=0){
		int a = computeL1index(originalAdr);
		int b = computeL2index(originalAdr);
		originalL2Data = L2[b][k].data;
		originalL1Data = L1[a].data;
		int originalL1tag = L1[a].tag;
		
		//Bring evicted L2 block to L1
		L1[a].data = L2[b][k].data;
		L1[a].tag = computeL1tag(originalAdr);
		L1[a].valid = true;
		L2[b][k].valid = false;

		//Bring evicted L1 block to victim
		int originalVictimTag;
		for(int i=0; i<VICTIM_SIZE; i++){
			if(victimCache[i].lru_position == 0){
				originalVictimTag = victimCache[i].tag;
				originalVictimData = victimCache[i].data;
				victimCache[i].data = originalL1Data;
				victimCache[i].lru_position = 3;
				victimCache[i].tag = (originalL1tag << 4) + a;
				victimCache[i].valid = true;
			}else{
				victimCache[i].lru_position--;
			}
		}

		//Bring evicted victim block to L2 and update LRU
		int lruBlock;
		int l2index = originalVictimTag % (1<<4);

		//Find least recently used block in set and update contents
		for(int i=0; i<L2_CACHE_WAYS; i++){
			if(L2[l2index][i].lru_position == 0){
				lruBlock = i;
				L2[l2index][lruBlock].lru_position = 7;
				L2[l2index][lruBlock].tag = (originalVictimTag >> 4) % (1<<26);
				L2[l2index][lruBlock].data = originalVictimData;
				L2[l2index][lruBlock].valid = true;
			}
		}

		//Decrement LRU position of every other block in set
		for(int i=0; i<L2_CACHE_WAYS; i++){
			if(i!=lruBlock){
				L2[l2index][i].lru_position--;
			}
		}
		return;
	}

	myStat.missL2++; //L2 miss

	//Must fetch from memory
	int a = computeL1index(originalAdr);
	int b = computeL2index(originalAdr);
	originalL1Data = L1[a].data;
	L1[a].data = myMem[originalAdr];
	bool originalValid = L1[a].valid;
	int originalL1tag = L1[a].tag;
	L1[a].valid = true;
	L1[a].tag = computeL1tag(originalAdr);

	if(!originalValid){
		return;
	}
	
	//Bring evicted L1 block to victim
	bool originalVictimValid;
	int originalVictimTag;
	for(int i=0; i<VICTIM_SIZE; i++){
			if(victimCache[i].lru_position == 0){
				originalVictimValid = victimCache[i].valid;
				originalVictimTag = victimCache[i].tag;
				originalVictimData = victimCache[i].data;
				victimCache[i].data = originalL1Data;
				victimCache[i].lru_position = 3;
				victimCache[i].tag = (originalL1tag << 4) + a;
				victimCache[i].valid = true;
			}else{
				victimCache[i].lru_position--;
			}
	}

	if(!originalVictimValid){
		return;
	}

	//Bring evicted victim block to L2 and update LRU
	int z;
	int l2index = originalVictimTag % (1<<4);
	for(int i=0; i<L2_CACHE_WAYS; i++){
		if(L2[l2index][i].lru_position == 0){
			z = i;
			L2[l2index][z].lru_position = L2_CACHE_WAYS-1;
			L2[l2index][z].tag = (originalVictimTag >> 4) % (1<<26);
			L2[l2index][z].valid = true;
			L2[l2index][z].data = originalVictimData;
			break;
		}
	}

	for(int i=0; i<L2_CACHE_WAYS; i++){
		if(i!=z){
			L2[l2index][i].lru_position--;
		}
	}
	
}

//If matching tag found in L1, return true. Otherwise return false.
bool cache::storeSearchL1(int* data, int adr, int* myMem){
	//2 bits for block offset, 4 bits for index, and 26 bits for tag
	int originalAdr = adr;
	int blockOffset = adr % 4;
	adr = adr >> 2;
	int index = adr % 16;
	adr = adr >> 4;
	int tag = adr % (1 << 26);
	if (L1[index].valid && L1[index].tag == tag){
		L1[index].data = *data; //Update data in cache
		myMem[originalAdr] = *data; //Update data in memory
		return true;
	}

	return false;
}

//If matching tag found in victim cache, return true. Otherwise return false.
bool cache::storeSearchVictim(int* data, int adr, int* myMem){
	//2 bits for block offset and 30 bits for tag
	int originalAdr = adr;
	int blockOffset = adr % 4;
	adr = adr >> 2;
	int tag = adr % (1<<30);
	for(int i=0; i<VICTIM_SIZE; i++){
		//Match found!
		if(victimCache[i].valid && victimCache[i].tag == tag){
			myMem[originalAdr] = *data; //Update memory

			//Bring evicted line from L1 to victim cache
			int originalVictimData = victimCache[i].data;
			victimCache[i].tag = computeVictimTag(originalAdr);
			victimCache[i].data = L1[computeL1index(originalAdr)].data;
			victimCache[i].valid = true;

			//Update victim cache LRU
			int originalVictimLRU = victimCache[i].lru_position;
			for(int i=0; i<VICTIM_SIZE; i++){
				if(victimCache[i].lru_position > originalVictimLRU){
					victimCache[i].lru_position--;
				}
			}
			victimCache[i].lru_position = 3;

			//Bring desired victim cache line to L1
			L1[computeL1index(originalAdr)].data = originalVictimData;
			adr = originalAdr >> 6;
			L1[computeL1index(originalAdr)].tag = adr % (1 << 26);
			L1[computeL1index(originalAdr)].valid = true;

			return true;
			}
	}

	return false;
}

bool cache::storeSearchL2(int* data, int adr, int* myMem){
	int originalAdr = adr;
	for(int i=0; i<L2_CACHE_WAYS; i++){
		if(L2[computeL2index(originalAdr)][i].valid && L2[computeL2index(adr)][i].tag == computeL2tag(adr)){
			int originalVictimData;
			myMem[originalAdr] = *data; //Update memory
			int a = computeL1index(originalAdr);
			int b = computeL2index(originalAdr);
			int originalL2Data = L2[b][i].data;
			int originalL1Data = L1[a].data;
		
			//Bring evicted L2 block to L1
			L1[a].data = L2[b][i].data;
			L1[a].tag = computeL1tag(originalAdr);
			L1[a].valid = true;

			//Bring evicted L1 block to victim and update LRUs
			for(int j=0; j<VICTIM_SIZE; j++){
				if(victimCache[j].lru_position == 0){
				originalVictimData = victimCache[j].data;
				victimCache[j].data = originalL1Data;
				victimCache[j].lru_position = 3;
				victimCache[j].tag = computeVictimTag(originalAdr);
				victimCache[j].valid = true;
				}else{
				victimCache[j].lru_position--;
				}
			}

			//Bring evicted victim block to L2 and update LRU
			L2[b][i].data = originalVictimData;
			int originalLRUposition = L2[b][i].lru_position;
			for(int k=0; k<L2_CACHE_WAYS; k++){
				if(L2[b][k].lru_position > originalLRUposition){
					L2[b][k].lru_position--;
				}
			}
			L2[b][i].lru_position = L2_CACHE_WAYS-1;

			return true;
		}
	}

	return false;
}


void cache::store(int* data, int adr, int* myMem){
	if(!storeSearchL1(data, adr, myMem) && !storeSearchVictim(data, adr, myMem) && !storeSearchL2(data, adr, myMem)){
		myMem[adr] = *data;
	}
}

void cache::controller(bool MemR, bool MemW, int *data, int adr, int *myMem)
{
	if (MemR)
	{
		load(data, adr, myMem);
	}else{
		//store(data, adr, myMem);
	}
}