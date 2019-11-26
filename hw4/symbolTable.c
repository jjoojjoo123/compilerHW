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
	SymbolTableEntry* firstEntry = symbolTable.hashTable[hashIndex];
	if(entry->name == firstEntry->name && entry->attribute->attributeKind == firstEntry->attribute->attributeKind && entry->nestingLevel == firstEntry->nestingLevel)
	{
		symbolTable.hashTable[hashIndex] = firstEntry->nextInHashChain;
		//firstEntry->nextInHashChain = NULL;
		free(firstEntry);
	}
	else
	{
		SymbolTableEntry* currentEntry = symbolTable.hashTable[hashIndex];
		while(entry->name != currentEntry->name || entry->attribute->attributeKind != currentEntry->attribute->attributeKind || entry->nestingLevel != currentEntry->nestingLevel)
		{
			currentEntry = currentEntry->nextInHashChain;
			if(!currentEntry)
			{
				printf("No such entry found!");
				exit(0);
			}
		}
		SymbolTableEntry* prevEntry = currentEntry->prevInHashChain;
		SymbolTableEntry* nextEntry = currentEntry->nextInHashChain;
		prevEntry->nextInHashChain = nextEntry;
		if(!nextEntry)	//到chain的尾巴 
			nextEntry->prevInHashChain = prevEntry;
		//currentEntry->prevInHashChain = NULL;
		//currentEntry->nextInHashChain = NULL;
		free(currentEntry);
	}
}

void enterIntoHashChain(int hashIndex, SymbolTableEntry* entry)
{
	SymbolTableEntry* firstEntry = symbolTable.hashTable[hashIndex];
	if(!firstEntry)		//這條chain是空的 
		symbolTable.hashTable[hashIndex] = entry;
	else
	{
		firstEntry->prevInHashChain = entry;
		entry->nextInHashChain = firstEntry;
		symbolTable.hashTable[hashIndex] = entry;
	}
}

void initializeSymbolTable()
{
	symbolTable.scopeDisplay = NULL;
	symbolTable.currentLevel = 0;
	symbolTable.scopeDisplayElementCount = 0;
}

void symbolTableEnd()
{
}

SymbolTableEntry* retrieveSymbol(char* symbolName)
{
}

SymbolTableEntry* enterSymbol(char* symbolName, SymbolAttribute* attribute)
{
	SymbolTableEntry* newEntry = newSymbolTableEntry(symbolTable.currentLevel);
	newEntry->name = symbolName;
	newEntry->attribute = attribute;
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
	symbolTable.scopeDisplayElementCount++;
}

void closeScope()
{
	symbolTable.currentLevel--;
}
