
#ifndef RF_H_
#define RF_H_

#include <avr/io.h>
#include <avr/interrupt.h>


void Trasmissor_Habilitar();
void Trasnmissor_desabilitar();
void Receptor_Habilitar();
void Receptor_desabilitar();
unsigned char Transmissor_envia(unsigned char* pacote, unsigned char tamanho);
unsigned char Receptor_recebe(unsigned char* dados, unsigned char* tamanho);
void Receptor_recebe_int( void (*function)(void));


#endif

