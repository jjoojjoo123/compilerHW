#include "symbolTable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// This file is for reference only, you are not required to follow the implementation. //

int HASH(char * str) {
    int idx=0;
    while (*str){
        idx = idx << 1;
        idx+=*str;
        str++;
    }
    return (idx & (HASH_TABLE_SIZE-1));
}

SymbolTable symbolTable;

SymbolTableEntry* newSymbolTableEntry(int nestingLevel)
{
    SymbolTableEntry* symbolTableEntry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
    symbolTableEntry->nextInHashChain = NULL;
    symbolTableEntry->prevInHashChain = NULL;
    symbolTableEntry->nextInSameLevel = NULL;
    symbolTableEntry->sameNameInOuterLevel = NULL;
    symbolTableEntry->attribute = NULL;
    symbolTableEntry->name = NULL;
    symbolTableEntry->nestingLevel = nestingLevel;
    return symbolTableEntry;
}

void removeFromHashChain(int hashIndex, SymbolTableEntry* entry)
{
    SymbolTableEntry* currentEntry = symbolTable.hashTable[hashIndex];
    SymbolTableEntry* sameNameInOuterLevel = NULL;
    if(strcmp(entry->name, currentEntry->name) == 0)
    {
        sameNameInOuterLevel = currentEntry->sameNameInOuterLevel;
        if(sameNameInOuterLevel){
            symbolTable.hashTable[hashIndex] = sameNameInOuterLevel;
            sameNameInOuterLevel->nextInHashChain = currentEntry->nextInHashChain;
            sameNameInOuterLevel->prevInHashChain = NULL;
            currentEntry->nextInHashChain->prevInHashChain = sameNameInOuterLevel;
        }else{
            symbolTable.hashTable[hashIndex] = currentEntry->nextInHashChain;
            if(currentEntry->nextInHashChain){
                currentEntry->nextInHashChain->prevInHashChain = currentEntry->nextInHashChain;
            }
        }
        free(currentEntry);
    }
    else
    {
        //SymbolTableEntry* currentEntry = symbolTable.hashTable[hashIndex];
        while(currentEntry && strcmp(entry->name, currentEntry->name) != 0)
        {
            currentEntry = currentEntry->nextInHashChain;
        }
        if(!currentEntry)
        {
            printf("No such entry found!");
            exit(0);
        }
        SymbolTableEntry* prevEntry = currentEntry->prevInHashChain;
        SymbolTableEntry* nextEntry = currentEntry->nextInHashChain;
        sameNameInOuterLevel = currentEntry->sameNameInOuterLevel;
        if(sameNameInOuterLevel){
            prevEntry->nextInHashChain = sameNameInOuterLevel;
            sameNameInOuterLevel->prevInHashChain = prevEntry;
            sameNameInOuterLevel->nextInHashChain = nextEntry;
            if(nextEntry){
                nextEntry->prevInHashChain = sameNameInOuterLevel;
            }
        }else{
            prevEntry->nextInHashChain = nextEntry;
            if(nextEntry){
                nextEntry->prevInHashChain = prevEntry;
            }
        }
        free(currentEntry);
    }
}

void enterIntoHashChain(int hashIndex, SymbolTableEntry* entry)
{
    SymbolTableEntry* currentEntry = symbolTable.hashTable[hashIndex];
    if(!currentEntry)     //
        symbolTable.hashTable[hashIndex] = entry;
    else if(strcmp(currentEntry->name, entry->name) == 0){
        entry->sameNameInOuterLevel = currentEntry; //you can detect redeclaration here
        symbolTable.hashTable[hashIndex] = entry;
    }
    else
    {
        while(strcmp(entry->name, currentEntry->name) != 0)
        {
            if(!currentEntry->nextInHashChain){
                currentEntry->nextInHashChain = entry;
                entry->prevInHashChain = currentEntry;
                return;
            }
            currentEntry = currentEntry->nextInHashChain;
        }
        entry->sameNameInOuterLevel = currentEntry;
        currentEntry->prevInHashChain->nextInHashChain = entry;
        entry->prevInHashChain = currentEntry->prevInHashChain;
        entry->nextInHashChain = currentEntry->nextInHashChain;
        if(currentEntry->nextInHashChain){
            currentEntry->nextInHashChain->prevInHashChain = entry;
        }
    }
}

void initializeSymbolTable()
{
    int i;
    for(i = 0;i < HASH_TABLE_SIZE;i++){
        symbolTable.hashTable[i] = NULL;
    }
    symbolTable.currentLevel = -1;
    symbolTable.scopeDisplayElementCount = 5; //maybe this indicates the total space? vector::capacity
    symbolTable.scopeDisplay = (SymbolTableEntry**)malloc(sizeof(SymbolTableEntry*) * symbolTable.scopeDisplayElementCount);
    //for ... = NULL;
    openScope();
    //symbolTable.scopeDisplay[0] = NULL;
}

void symbolTableEnd()
{
}

SymbolTableEntry* retrieveSymbol(char* symbolName)
{
}

SymbolTableEntry* enterSymbol(char* symbolName, SymbolAttribute* attribute)
{
    //symbolTable.scopeDisplayElementCount++;
    SymbolTableEntry* newEntry = newSymbolTableEntry(symbolTable.currentLevel);
    newEntry->nextInSameLevel = symbolTable.scopeDisplay[symbolTable.currentLevel];
    symbolTable.scopeDisplay[symbolTable.currentLevel] = newEntry;
    newEntry->name = symbolName;
    newEntry->attribute = attribute;
    enterIntoHashChain(HASH(symbolName), newEntry);
    return newEntry;
}

//remove the symbol from the current scope
void removeSymbol(char* symbolName)
{
}

int declaredLocally(char* symbolName)
{
}

void openScope()
{
    //...
    symbolTable.currentLevel++;
    if(symbolTable.currentLevel >= symbolTable.scopeDisplay){
        symbolTableEntry** newScopeDisplay = (SymbolTableEntry**)malloc(sizeof(SymbolTableEntry*) * symbolTable.scopeDisplayElementCount * 2);
        memcpy(newScopeDisplay, symbolTable.scopeDisplay, sizeof(SymbolTableEntry*) * symbolTable.scopeDisplayElementCount);
        symbolTable.scopeDisplayElementCount *= 2;
        free(symbolTable.scopeDisplay);
        symbolTable.scopeDisplay = newScopeDisplay;
    }
    symbolTable.scopeDisplay[symbolTable.currentLevel] = NULL;
}

void closeScope()
{
    symbolTable.currentLevel--;
    //scopeDisplayElementCount = 0;
}