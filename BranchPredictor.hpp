#ifndef __BRANCH_PREDICTOR_HPP__
#define __BRANCH_PREDICTOR_HPP__

#include <vector>
#include <bitset>
#include <cassert>
#include <iostream>
using namespace std ;

struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> table;
    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        if (table[pc%(1<<14)] == 2 || table[pc%(1<<14)] == 3) return true ; 
        return false; 
    }

    void update(uint32_t pc, bool taken) {
        if (table[pc%(1<<14)] == 3 && not taken) table[pc%(1<<14)] = 2 ;
        else if (table[pc%(1<<14)] == 2 && not taken) table[pc%(1<<14)] = 1 ;
        else if (table[pc%(1<<14)] == 1 && not taken) table[pc%(1<<14)] = 0 ;
        else if (table[pc%(1<<14)] == 0 && taken) table[pc%(1<<14)] = 1 ;
        else if (table[pc%(1<<14)] == 1 && taken) table[pc%(1<<14)] = 2 ;
        else if (table[pc%(1<<14)] == 2 && taken) table[pc%(1<<14)] = 3 ;
    }
};

struct BHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        int bhr_int  = bhr.to_ulong() ;
        if (bhrTable[bhr_int] == 2 || bhrTable[bhr_int] == 3) return true ;
        return false;
    }

    void update(uint32_t pc, bool taken) {
        int bhr_int = bhr.to_ulong() ;
        if (bhrTable[bhr_int] == 3 && not taken) bhrTable[bhr_int] = 2 ;
        else if (bhrTable[bhr_int] == 2 && not taken) bhrTable[bhr_int] = 1 ;
        else if (bhrTable[bhr_int] == 1 && not taken) bhrTable[bhr_int] = 0 ;
        else if (bhrTable[bhr_int] == 0 && taken) bhrTable[bhr_int] = 1 ;
        else if (bhrTable[bhr_int] == 1 && taken) bhrTable[bhr_int] = 2 ;
        else if (bhrTable[bhr_int] == 2 && taken) bhrTable[bhr_int] = 3 ;
        bhr[1] = bhr[0];
        bhr[0] = taken ;   
         }
};

struct SaturatingBHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    std::vector<std::bitset<2>> table;
    std::vector<std::bitset<2>> combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));
    }

    bool predict(uint32_t pc) {
        int bhr_int = bhr.to_ulong() ;
        if (combination[(pc%(1<<14))] ==3) return true ;
        else if (combination[(pc%(1<<14))]==0) return false ;
        else if (bhr_int == 0) return false ;
        else if (bhr_int == 3) return true ;
        else if (combination[(pc%(1<<14))]==2) return true ;
        return false;
    }

    void update(uint32_t pc, bool taken) {
        bhr[1] = bhr[0];
        bhr[0] = taken ; 
        int bhr_int = bhr.to_ulong() ;
        if (combination[pc%(1<<14)] == 3 && not taken) combination[pc%(1<<14)] = 2 ;
        else if (combination[pc%(1<<14)] == 2 && not taken) combination[pc%(1<<14)] = 1 ;
        else if (combination[pc%(1<<14)] == 1 && not taken) combination[pc%(1<<14)] = 0 ;
        else if (combination[pc%(1<<14)] == 0 && taken) combination[pc%(1<<14)] = 1 ;
        else if (combination[pc%(1<<14)] == 1 && taken) combination[pc%(1<<14)] = 2 ;
        else if (combination[pc%(1<<14)] == 2 && taken) combination[pc%(1<<14)] = 3 ;
    }
};

#endif