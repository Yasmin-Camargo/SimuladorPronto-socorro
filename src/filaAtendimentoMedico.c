//FILA DINÂMICA FIFO EM C

#include "filaAtendimentoMedico.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct Fila {
	int numero;
    struct Fila *next;  
} Fila;

Fila *first;
Fila *last;

//Escopo de funções
void Reset_filaAtendimentoMedico();
void Push_filaAtendimentoMedico(int num);
int Pop_filaAtendimentoMedico();
void Clear_filaAtendimentoMedico();

void Reset_filaAtendimentoMedico(){
    last = NULL;
    first = NULL;
}

void Push_filaAtendimentoMedico(int num){   
    Fila *minhaFilaNova;
    minhaFilaNova = (Fila *) malloc(sizeof(Fila));
    minhaFilaNova->numero = num;
    minhaFilaNova->next = NULL;

    if (first == NULL){
        first = minhaFilaNova;
        last = minhaFilaNova;
    }
    else{
        last->next = minhaFilaNova;
        last = minhaFilaNova;
        minhaFilaNova->next = NULL;
    }
}

int Pop_filaAtendimentoMedico(){
    int numRemov = 0;
    if (first == NULL){ //Fila vazia
        return -1;
    }
    else{
        Fila *aux;
        aux = first;
        numRemov = first->numero;
        first = first->next;
        free(aux);
        return (numRemov);
    }
}

void Print_filaAtendimentoMedico(){
     printf("\n ");
    for (Fila *i = first; i != NULL; i = i->next){
        printf("\n %d", i->numero);
    }
}

void Clear_filaAtendimentoMedico(){
    while (first != NULL){
        Pop_filaAtendimentoMedico();
    }
}

