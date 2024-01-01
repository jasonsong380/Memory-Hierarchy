#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>
#include "cache.h"
#include <iomanip>
using namespace std;

struct trace
{
	bool MemR; 
	bool MemW; 
	int adr; 
	int data; 
};

int main (int argc, char* argv[])
{
	// input file (i.e., test.txt)
	string filename = argv[1];
	
	ifstream fin;

	// opening file
	fin.open(filename.c_str());
	if (!fin){ // making sure the file is correctly opened
		cout << "Error opening " << filename << endl;
		exit(1);
	}
	
	// reading the text file
	string line;
	vector<trace> myTrace;
	int TraceSize = 0;
	string s1,s2,s3,s4;
    while( getline(fin,line) )
      	{
            stringstream ss(line);
            getline(ss,s1,','); 
            getline(ss,s2,','); 
            getline(ss,s3,','); 
            getline(ss,s4,',');
            myTrace.push_back(trace()); 
            myTrace[TraceSize].MemR = stoi(s1);
            myTrace[TraceSize].MemW = stoi(s2);
            myTrace[TraceSize].adr = stoi(s3);
            myTrace[TraceSize].data = stoi(s4);
            TraceSize+=1;
        }


	// Defining cache and stat
    cache myCache;
    int myMem[MEM_SIZE]; 


	int traceCounter = 0;
	bool cur_MemR; 
	bool cur_MemW; 
	int cur_adr;
	int cur_data;

	// this is the main loop of the code
	while(traceCounter < TraceSize){
		
		cur_MemR = myTrace[traceCounter].MemR;
		cur_MemW = myTrace[traceCounter].MemW;
		cur_data = myTrace[traceCounter].data;
		cur_adr = myTrace[traceCounter].adr;
		traceCounter += 1;
		myCache.controller (cur_MemR, cur_MemW, &cur_data, cur_adr, myMem); // in your memory controller you need to implement your FSM, LW, SW, and MM. 
	}
	
	long double L1_miss_rate, L2_miss_rate, AAT; 
	//compute the stats here:
	L1_miss_rate = myCache.getL1MissRate();
	L2_miss_rate = myCache.getL2MissRate();
	long double V_miss_rate = myCache.getVictimMissRate();
	long double hitTimeL1 = 1;
	long double hitTimeVictim = 1;
	long double hitTimeL2 = 8;
	long double hitTimeMain = 100;
	long double MP_V = hitTimeL2+L2_miss_rate*hitTimeMain;
	AAT = hitTimeL1+L1_miss_rate*(hitTimeVictim+V_miss_rate*MP_V);

	cout<< setprecision(10);
	cout<< "(" << L1_miss_rate<<","<<L2_miss_rate<<","<<AAT<<")"<<endl;

	// closing the file
	fin.close();

	return 0;
}
