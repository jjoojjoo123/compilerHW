#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"header.h"

#define TABLE_SIZE	512

symtab * hash_table[TABLE_SIZE];
extern int linenumber;

int HASH(char * str){
	int idx=0;
	while(*str){
		idx = idx << 1;
		idx+=*str;
		str++;
	}	
	return (idx & (TABLE_SIZE-1));
}

/*returns the symbol table entry if found else NULL*/

symtab * lookup(char *name){
	int hash_key;
	symtab* symptr;
	if(!name)
		return NULL;
	hash_key=HASH(name);
	symptr=hash_table[hash_key];

	while(symptr){
		if(!(strcmp(name,symptr->lexeme)))
			return symptr;
		symptr=symptr->front;
	}
	return NULL;
}


void insertID(char *name){
	int hash_key;
	symtab* ptr;
	symtab* symptr=(symtab*)malloc(sizeof(symtab));	
	
	hash_key=HASH(name);
	ptr=hash_table[hash_key];
	
	if(ptr==NULL){
		/*first entry for this hash_key*/
		hash_table[hash_key]=symptr;
		symptr->front=NULL;
		symptr->back=symptr;
	}
	else{
		symptr->front=ptr;
		ptr->back=symptr;
		symptr->back=symptr;
		hash_table[hash_key]=symptr;	
	}
	
	strcpy(symptr->lexeme,name);
	symptr->line=linenumber;
	symptr->counter=1;
}

void printSym(symtab* ptr) 
{
	    printf(" Name = %s \n", ptr->lexeme);
	    printf(" References = %d \n", ptr->counter);
}

void printSymTab()
{
    int i;
    printf("----- Symbol Table ---------\n");
    for (i=0; i<TABLE_SIZE; i++)
    {
        symtab* symptr;
	symptr = hash_table[i];
	while (symptr != NULL)
	{
            printf("====>  index = %d \n", i);
	    printSym(symptr);
	    symptr=symptr->front;
	}
    }
}

symtab *insertOrder(symtab *root, symtab *new){
	if(root == NULL){
		return new;
	}else if(strcmp(root->lexeme, new->lexeme) > 0){
		new->front = root;
		new->back = root->back;
		root->back = new;
		/*if(new->back != NULL){
			new->back->front = new;
		}*/
		return new;
	}else{
		if(root->front == NULL){
			root->front = new;
			new->back = root;
		}else{
			root->front = insertOrder(root->front, new);
		}
		return root;
	}
}

void printFreq(){
	int i;
	int size = 0;
	symtab *root = NULL, *ptr = NULL;
	for (i=0; i<TABLE_SIZE; i++){
		symtab* symptr;
		symptr = hash_table[i];
		while (symptr != NULL){
			symtab *new = (symtab*)malloc(sizeof(symtab));
			strcpy(new->lexeme, symptr->lexeme);
			new->line = symptr->line;
			new->counter = symptr->counter;
			root = insertOrder(root, new);
			symptr=symptr->front;
		}
	}
	ptr = root;
	while(ptr != NULL){
		printf("%s\t%d\n", ptr->lexeme, ptr->counter);
		ptr = ptr->front;
	}
}