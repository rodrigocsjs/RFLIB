#include "RF.h"

static unsigned char RECEPTORAMOSTRA = 0x00, RECEPTORULTAMOSTRA = 0x00, RECEPTORSYCQTD = 0x00;
static unsigned char RECEPTORCICLOAMST = 0x00, RECEPTORDADO[32], RECEPTORHABI= 0x00;
static unsigned char RECEPTORBITQTD = 0x00, RECEPTORDADOQTD = 0x00, RECEPTOROCUPAD = 0x00;
static unsigned int RECEPTORUL12BITREC = 0x00, TRANSMISSODADOQTD = 0x00;
static volatile unsigned char RECEPTORNOVAMSG = 0x00, RECEPTORTAMMSGREC = 0x00, TRANSMISSOHAB = 0x00;
static unsigned char TRANSMISSORDADOQTD = 0x00, TRANSMISSORDADOPRX = 0x00 ,TRANSMISSORBITPRX = 0x00,  TRANSMISSORAMOSTRAQTD = 0x00;
static unsigned char TRANSMISSORDADO[72] = {0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,0x38,0x2C};
static unsigned char TABELA[16] ={0x0D,0x0E,0x13,0x15,0x16,0x19,0x1A,0x1C,0x23,0x25,0x26,0x29,0x2A,0x2C,0x32,0x34};
static __inline__ unsigned int calculocrc(unsigned int __crc, unsigned char __data);
static void (*FUNCAO_INT)(void);
unsigned char converte(unsigned char byte);
unsigned int receptorCRC(unsigned char tam, unsigned char *pacote);
static volatile char recebe_int =0x00;

static unsigned char Transmissor_pino = PINB0;
static unsigned char Receptor_pino =    PINB1;

//===================================================================================================================
void Receptor_recebe_int(void (*function)(void))
{
	recebe_int =1;
	FUNCAO_INT = function;
}
//===================================================================================================================
void recebedados()
{
	unsigned char dados =0x00;
	if (RECEPTOROCUPAD == 0x01)
	{
		if (++RECEPTORBITQTD >= 0x0C)
		{
			dados = (converte( RECEPTORUL12BITREC & 0x3F) ) << 0x04 | converte(RECEPTORUL12BITREC >> 0x06);
			if (RECEPTORTAMMSGREC == 0)
			{
				RECEPTORDADOQTD = dados;
				if ( (RECEPTORDADOQTD < 0x04) || (RECEPTORDADOQTD > 0x20) )	{RECEPTOROCUPAD = 0; return;}
			}
			RECEPTORDADO[RECEPTORTAMMSGREC++] = dados;
			if (RECEPTORTAMMSGREC >= RECEPTORDADOQTD) 
			{
				RECEPTOROCUPAD = 0; 
				RECEPTORNOVAMSG = 1;
				if(recebe_int == 1)
				{
					FUNCAO_INT();
				}
							
			}
			RECEPTORBITQTD = 0x00;
		}
	}
	else
	{
		if (RECEPTORUL12BITREC == 0x0B38)
		{
			RECEPTOROCUPAD = 0x01; RECEPTORBITQTD = 0x00;	RECEPTORTAMMSGREC = 0x00; RECEPTORNOVAMSG = 0x00;
		}
	}
}

//==================================================================================================================
void carregadados()
{
	if (RECEPTORAMOSTRA != RECEPTORULTAMOSTRA)
	{
		RECEPTORSYCQTD += ((RECEPTORSYCQTD < 0x50) ? 0x0B : 0x1D);
		RECEPTORULTAMOSTRA = RECEPTORAMOSTRA;
	}
	else
	{
		RECEPTORSYCQTD = (RECEPTORSYCQTD + 0x14);
	}
	
	if(RECEPTORAMOSTRA)	RECEPTORCICLOAMST++;
	if(RECEPTORSYCQTD >= 0xA0)
	{
		RECEPTORUL12BITREC = (RECEPTORCICLOAMST >= 0x05) ? ( (RECEPTORUL12BITREC >> 1 ) | 0x800 ):(RECEPTORUL12BITREC >> 1);
		RECEPTORSYCQTD = (RECEPTORSYCQTD - 0xA0); RECEPTORCICLOAMST = 0x00;
		recebedados();
	}
}

//============================================================================================================
void RF_Habilitar()
{
	TCCR1A = 0x00;
	TCCR1B |= (1<<WGM12)|(1<<CS10);
	OCR1A = 1000;
	TIMSK1 |= (1 << OCIE1A);
	sei();
}
//============================================================================================================
void Trasmissor_Habilitar()
{
	DDRB |= (1 << Transmissor_pino);
	 RF_Habilitar();
	TRANSMISSORDADOPRX = 0x00;	TRANSMISSORBITPRX = 0x00;
	TRANSMISSORAMOSTRAQTD = 0x00;	TRANSMISSOHAB = 0x01;
}
//============================================================================================================

void Trasnmissor_desabilitar()
{
	PORTB &= ~(1 << Transmissor_pino);
	TCCR1B=0x00;
	TIMSK1 &= ~(1 << OCIE1A);
	TRANSMISSOHAB = 0x00;
}
//============================================================================================================

void Receptor_Habilitar()
{
	if (RECEPTORHABI ==0x00 )
	{
		DDRB &= ~(1 << Receptor_pino );
		PORTB |=(1<<Receptor_pino );
		RECEPTORHABI = 0x01; RECEPTOROCUPAD = 0x00;
		RF_Habilitar();
	}
}

//============================================================================================================

void Receptor_desabilitar()
{
	if (RECEPTORHABI ==0x01 )
	{
		TCCR1B=0x00;
		TIMSK1 &= ~(1 << OCIE1A);
		PORTB &= ~(1<<Receptor_pino );
		RECEPTORHABI = 0x00; 	
	}
}
//================================================================================================================

unsigned char converte(unsigned char byte)
{
	unsigned char meiobyte =0x00;
	do
	{
		if (byte == TABELA[meiobyte])
		{
			return meiobyte;
		}
		
		meiobyte++;
		
	} while (meiobyte < 16 );
	
	return 0;
}

//============================================================================================================

unsigned char Transmissor_envia(unsigned char* pacote, unsigned char tamanho)
{

	if(tamanho > 0x1D) return 0;
	while(TRANSMISSOHAB == 1)
	{
		;
	}
	unsigned char *mensagem = TRANSMISSORDADO + 0x08;
	unsigned char IDmsg = 0x00, IDtabela = tamanho + 0x03, IDpacote =0x00;
	unsigned int Transmissorcrc = 0xFFFF;
	
	Transmissorcrc = calculocrc(Transmissorcrc, IDtabela);
	mensagem[IDmsg++] = TABELA[IDtabela >> 0x04];	mensagem[IDmsg++] = TABELA[IDtabela & 0x0F];
	do
	{
		Transmissorcrc = calculocrc(Transmissorcrc, pacote[IDpacote]);
		mensagem[IDmsg++] = TABELA[pacote[IDpacote] >> 0x04]; mensagem[IDmsg++] = TABELA[pacote[IDpacote] & 0x0F];
		IDpacote++;
		
	} while(IDpacote < tamanho);
	
	Transmissorcrc = ~Transmissorcrc;
	mensagem[IDmsg++] = TABELA[(Transmissorcrc >> 0x04)  & 0x0F];	mensagem[IDmsg++] = TABELA[Transmissorcrc & 0x0F];
	mensagem[IDmsg++] = TABELA[(Transmissorcrc >> 0x0C) & 0x0F];	mensagem[IDmsg++] = TABELA[(Transmissorcrc >> 0X08)  & 0x0F];
	TRANSMISSORDADOQTD = IDmsg + 8;
	Trasmissor_Habilitar();
	
	return 1;
}

//============================================================================================================

void memcp(void* dest, void* src, int size)
{
	unsigned char *pdest = (unsigned char*) dest;
	unsigned char *psrc = (unsigned char*) src;

	int loops = (size / 4);
	for(int index = 0; index < loops; ++index)
	{
		*((long*)pdest) = *((long*)psrc);
		pdest += 4;
		psrc += 4;
	}

	loops = (size % 4);
	for (int index = 0; index < loops; ++index)
	{
		*pdest = *psrc;
		++pdest;
		++psrc;
	}
}

//============================================================================================================
unsigned int receptorCRC(unsigned char tam, unsigned char *pacote)
{
	unsigned int recepcrc = 0xFFFF;
	for (; tam > 0 ; tam--)
	{
		recepcrc = calculocrc(recepcrc, *pacote++);
	}
	return recepcrc;
}
//============================================================================================================

unsigned char Receptor_recebe(unsigned char* dados, unsigned char* tamanho)
{
	
	if (RECEPTORNOVAMSG == 0)return 0;
	*tamanho = (*tamanho > (RECEPTORTAMMSGREC - 0x03) )?(RECEPTORTAMMSGREC - 0x03):(*tamanho);
	memcp(dados, RECEPTORDADO + 1, *tamanho);
	RECEPTORNOVAMSG = 0;
	return (receptorCRC(RECEPTORTAMMSGREC,RECEPTORDADO) == 0xF0B8);
}
//============================================================================================================


ISR(TIMER1_COMPA_vect)
{

	if(RECEPTORHABI && !TRANSMISSOHAB)
	{
		RECEPTORAMOSTRA = (PINB & (1<< Receptor_pino ));
	}
	
	if (TRANSMISSOHAB && TRANSMISSORAMOSTRAQTD++ == 0)
	{
		
		if (TRANSMISSORDADOPRX >= TRANSMISSORDADOQTD)
		{
			Trasnmissor_desabilitar(); 	TRANSMISSODADOQTD = (TRANSMISSODADOQTD + 1);
		}
		else
		{
			if(TRANSMISSORDADO[TRANSMISSORDADOPRX] & (1 << TRANSMISSORBITPRX++) )
			{
				PORTB |= (1 << Transmissor_pino);
			}
			else
				{
					PORTB &= ~(1 << Transmissor_pino);
				}
			
			if (TRANSMISSORBITPRX >= 0x06)
			{
				TRANSMISSORBITPRX = 0x00; 	TRANSMISSORDADOPRX = (TRANSMISSORDADOPRX + 1);
			}
		}
	}
	if (TRANSMISSORAMOSTRAQTD > 0x07)
	{
		TRANSMISSORAMOSTRAQTD = 0;
	}
	if (RECEPTORHABI && !TRANSMISSOHAB)
	{
		carregadados();
	}
	
}

//============================================================================================================
static __inline__ unsigned int calculocrc(unsigned int __crc, unsigned char __data)
{
	unsigned int __ret;
	__asm__ __volatile__ (
	"eor    %A0,%1"          "\n\t"
	"mov    __tmp_reg__,%A0" "\n\t"
	"swap   %A0"             "\n\t"
	"andi   %A0,0xf0"        "\n\t"
	"eor    %A0,__tmp_reg__" "\n\t"
	"mov    __tmp_reg__,%B0" "\n\t"
	"mov    %B0,%A0"         "\n\t"
	"swap   %A0"             "\n\t"
	"andi   %A0,0x0f"        "\n\t"
	"eor    __tmp_reg__,%A0" "\n\t"
	"lsr    %A0"             "\n\t"
	"eor    %B0,%A0"         "\n\t"
	"eor    %A0,%B0"         "\n\t"
	"lsl    %A0"             "\n\t"
	"lsl    %A0"             "\n\t"
	"lsl    %A0"             "\n\t"
	"eor    %A0,__tmp_reg__"
	: "=d" (__ret)
	: "r" (__data), "0" (__crc)
	: "r0"
	);
	return __ret;
}
//============================================================================================================