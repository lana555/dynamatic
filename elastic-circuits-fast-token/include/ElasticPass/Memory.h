#pragma once
#include "Head.h"

int getBBIndex(ENode* enode);
int getMemPortIndex(ENode* enode);
int getBBOffset(ENode* enode);
int getMemLoadCount(ENode* memnode);
int getMemStoreCount(ENode* memnode);
int getMemBBCount(ENode* memnode);
int getMemInputCount(ENode* memnode);
int getMemOutputCount(ENode* memnode);
int getDotAdrIndex(ENode* enode, ENode* memnode);
int getDotDataIndex(ENode* enode, ENode* memnode);
int getBBLdCount(ENode* enode, ENode* memnode);
int getBBStCount(ENode* enode, ENode* memnode);
int getLSQDepth(ENode* memnode);
void setMemPortIndex(ENode* enode, ENode* memnode);
void setLSQMemPortIndex(ENode* enode, ENode* memnode);
void setBBOffset(ENode* enode, ENode* memnode);
bool compareMemInst(ENode* enode1, ENode* enode2);
