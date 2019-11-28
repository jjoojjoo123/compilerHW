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
    if(!entry->prevInHashChain){
        symbolTable.hashTable[hashIndex] = entry->nextInHashChain;
        if(entry->nextInHashChain){
            entry->nextInHashChain->prevInHashChain = NULL;
        }
    }
    entry->prevInHashChain = NULL;
    entry->nextInHashChain = NULL;
}

void enterIntoHashChain(int hashIndex, SymbolTableEntry* entry)
{
    if(!symbolTable.hashTable[hashIndex]){
        symbolTable.hashTable[hashIndex] = entry;
    }else{
        symbolTable.hashTable[hashIndex]->prevInHashChain = entry;
        entry->nextInHashChain = symbolTable.hashTable[hashIndex];
        symbolTable.hashTable[hashIndex] = entry;
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
    while(symbolTable.currentLevel > -1){
        closeScope();
    }
    free(symbolTable.scopeDisplay);
}

SymbolTableEntry* retrieveSymbol(char* symbolName)
{
    SymbolTableEntry* currentEntry = symbolTable.hashTable[HASH(symbolName)];
    while(currentEntry){
        if(strcmp(currentEntry->name, symbolName) == 0){
            return currentEntry;
        }
        currentEntry = currentEntry->nextInHashChain;
    }
    return NULL;
}

SymbolTableEntry* enterSymbol(char* symbolName, SymbolAttribute* attribute)
{
    //symbolTable.scopeDisplayElementCount++;
    int hashIndex = HASH(symbolName);
    SymbolTableEntry* newEntry = newSymbolTableEntry(symbolTable.currentLevel);
    newEntry->nextInSameLevel = symbolTable.scopeDisplay[symbolTable.currentLevel];
    symbolTable.scopeDisplay[symbolTable.currentLevel] = newEntry;
    newEntry->name = symbolName;
    newEntry->attribute = attribute;
    SymbolTableEntry* currentEntry = symbolTable.hashTable[hashIndex];
    while(currentEntry){
        if(strcmp(currentEntry->name, symbolName) == 0){
            if(currentEntry->nestingLevel == symbolTable.currentLevel){
                //redeclaration!!!
                symbolTable.scopeDisplay[symbolTable.currentLevel] = newEntry->nextInSameLevel; //roll back
                free(newEntry);
                return NULL;
            }
            newEntry->sameNameInOuterLevel = currentEntry;
            removeFromHashChain(hashIndex, currentEntry);
            break;
        }
        currentEntry = currentEntry->nextInHashChain;
    }
    enterIntoHashChain(hashIndex, newEntry);
    return newEntry;
}

//remove the symbol from the current scope
void removeSymbol(char* symbolName)
{
    int hashIndex = HASH(symbolName);
    SymbolTableEntry* currentEntry = symbolTable.hashTable[hashIndex];
    while(currentEntry){
        if(strcmp(currentEntry->name, symbolName) == 0){
            if(currentEntry->nestingLevel != symbolTable.currentLevel){
                //nothing to remove
                return;
            }else{
                removeFromHashChain(hashIndex, currentEntry);
                break;
            }
        }
        currentEntry = currentEntry->nextInHashChain;
    }
    if(!currentEntry){
        //nothing to remove
        return;
    }
    if(currentEntry->sameNameInOuterLevel){
        enterIntoHashChain(hashIndex, currentEntry->sameNameInOuterLevel);
    }
    //scopeDisplay
    SymbolTableEntry* currentScopeEntry = symbolTable.scopeDisplay[symbolTable.currentLevel];
    if(currentScopeEntry == currentEntry){
        symbolTable.scopeDisplay[symbolTable.currentLevel] = currentEntry->nextInSameLevel;
    }else{
        while(currentScopeEntry && currentScopeEntry->nextInSameLevel != currentEntry){
            currentScopeEntry = currentScopeEntry->nextInSameLevel;
        }
        currentScopeEntry->nextInSameLevel = currentEntry->nextInSameLevel;
    }
    free(currentEntry);
}

int declaredLocally(char* symbolName)
{
    SymbolTableEntry* currentEntry = symbolTable.hashTable[HASH(symbolName)];
    while(currentEntry){
        if(strcmp(currentEntry->name, symbolName) == 0){
            return (currentEntry->nestingLevel == symbolTable.currentLevel) ? 1 : 0;
        }
        currentEntry = currentEntry->nextInHashChain;
    }
    return 0;
}

void openScope()
{
    //...
    symbolTable.currentLevel++;
    if(symbolTable.currentLevel >= symbolTable.scopeDisplayElementCount){
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
    //we make sure that all names in the current scope are on the chain, and no same name in a scope
    SymbolTableEntry* currentEntry = symbolTable.scopeDisplay[symbolTable.currentLevel];
    while(currentEntry){
        int hashIndex = HASH(currentEntry->name);
        removeFromHashChain(hashIndex, currentEntry);
        if(currentEntry->sameNameInOuterLevel){
            enterIntoHashChain(hashIndex, currentEntry->sameNameInOuterLevel);
        }
        //free(current)?
        currentEntry = currentEntry->nextInSameLevel;
    }
    symbolTable.currentLevel--;
    //scopeDisplayElementCount = 0;
}