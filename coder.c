/*
 * WSPR Toolkit for Linux
 * ======================
 * by DJ0ABR 
 * 
 * coder.c 
 * 
 * this program generates a WSPR data stream
 * 
 * first generate the 162 WSPR symbols in makeWSPRsamples
 * then convert the symbols to soundcard-samples "txsamples"
 * 
 * */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "config.h"
#include "soundcard.h"
#include "kmtools.h"
#include "coder.h"

void makeWSPRframe(char* call, char* l, int dbm, unsigned char* symbols);
void makeWSPRsamples(unsigned char* symbols);
void init_symfreq();

short txsamples[WSPR_RATE*MAXSECONDS];	// tx Samples für das komplette WSPR Intervall

void wsprcoder()
{
unsigned char sym[162];

	// Fülle sym mit den WSPR Symbolen
    init_symfreq();
	makeWSPRframe(callsign,qthloc,txpower,sym);
	makeWSPRsamples(sym);
}

float symfreq[4];	// WSPR Frequenzen
float TAU = 2*M_PI;

void init_symfreq()
{
float space = 12000.0 / 8192.0;

	symfreq[0] = (float)txoffset - space - space / 2;
	symfreq[1] = (float)txoffset - space / 2;
	symfreq[2] = (float)txoffset + space / 2;
	symfreq[3] = (float)txoffset + space + space / 2;
}

void makeWSPRsamples(unsigned char* symbols)
{
    
    
	int sampleanz = 8196; // = (int)((float)wspr_rate * (float)683 / (float)1000); // 683 = ms per symbol
	// 'volume' is UInt16 with range 0 thrugh Uint16.MaxValue ( = 65535)
	// we need 'amp' to have the range of 0 thru Int16.MaxValue ( = 32767)
	int volume = 15000;	// nur ein Beispiel
	double amp = volume >> 2; // so we simply set amp = volume / 2

	int idx = 0;
	for (int symnummer = 0; symnummer < 162; symnummer++)
	{
		float f = symfreq[symbols[symnummer]];
		//info.diag("EXT.TRX", symnummer.ToString() + " " + f.ToString());
		float theta = f * TAU / (float)WSPR_RATE;
		for (int step = 0; step < sampleanz; step++)
		{
			short s = (short)(amp * sin(theta * (float)step));
			if(idx < (WSPR_RATE*MAXSECONDS))
				txsamples[idx++] = s;
			else
				break;
		}
	}          
}

// symbol muss 162 Byte lang sein
void makeWSPRframe(char* call, char* l, int dbm, unsigned char* symbols)
{

	// calculate the WSPR string
	// input: call,locator,power
	// output: symbols (string of symbols 0,1,2,3 for the four frequencies 0=lowest frequency 3=highest)
	// a part of these functions are from the Raspberry PI WSPR project, and heavily debugged and corrected
	int n;
	long i;
	int p = dbm;    //EIRP in dBm={0,3,7,10,13,17,20,23,27,30,33,37,40,43,47,50,53,57,60} 
	int corr[]={0,-1,1,0,-1,2,1,0,-1,1};  

	unsigned char symbol[176];   
	unsigned long int n1,n2;    
	int k = 0;
	int j,s;
	long nstate = 0; 
	char packed[11]; 
	unsigned long ng = 0,nadd=0;

	// interleave symbols
	const unsigned char npr3[162] = {
	  1,1,0,0,0,0,0,0,1,0,0,0,1,1,1,0,0,0,1,0,0,1,0,1,1,1,1,0,0,0,0,0,
	  0,0,1,0,0,1,0,1,0,0,0,0,0,0,1,0,1,1,0,0,1,1,0,1,0,0,0,1,1,0,1,0,
	  0,0,0,1,1,0,1,0,1,0,1,0,1,0,0,1,0,0,1,0,1,1,0,0,0,1,1,0,1,0,1,0,
	  0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,1,1,0,1,1,0,0,1,1,0,1,0,0,0,1,1,1,
	  0,0,0,0,0,1,0,1,0,0,1,1,0,0,0,0,0,0,0,1,1,0,1,0,1,1,0,0,0,1,1,0,
	  0,0 };

	// pack prefix in nadd, call in n1, grid, dbm in n2 
	char* c, buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	strncpy(buf, call, 16);
	c=buf;
	strupr(c);             

	if(strchr(c, '/')){ //prefix-suffix
	 nadd=2;
	 i=strchr(c, '/')-c; //stroke position 
	 n=strlen(c)-i-1; //suffix len, prefix-call len
	 c[i]='\0';
	 if(n==1) ng = 60000L - 32768L + (c[i+1]>='0'&&c[i+1]<='9' ? (unsigned long)(c[i+1]-'0') : c[i+1]==' ' ? 38L : (unsigned long)c[i+1]-'A'+10L); // suffix /A to /Z, /0 to /9
	 if(n==2) ng=60000L + 26L + 10L * ((unsigned long)(c[i+1]-'0'))+((unsigned long)(c[i+2]-'0')); // suffix /10 to /99
	 if(n>2){ // prefix EA8/, right align
	   ng = (i<3 ? 36L : c[i-3]>='0' && c[i-3]<='9' ? (unsigned long)(c[i-3]-'0') : (unsigned long)c[i-3]-'A'+10L);
	   ng = 37L * ng + (i<2 ? 36L : c[i-2]>='0'&&c[i-2]<='9' ? (unsigned long)(c[i-2]-'0') : (unsigned long)c[i-2]-'A'+10L);
	   ng = 37L * ng + (i<1 ? 36L : c[i-1]>='0'&&c[i-1]<='9' ? (unsigned long)(c[i-1]-'0') : (unsigned long)c[i-1]-'A'+10L);
	   if(ng<32768L) nadd=1; else ng=ng-32768L;
	   c=c+i+1;
	 }
	}    



	i=(isdigit(c[2])?2:isdigit(c[1])?1:0); //last prefix digit of de-suffixed/de-prefixed callsign
	n=strlen(c)-i-1; //2nd part of call len

	n1=(i<2 ? 36L : c[i-2]>='0'&&c[i-2]<='9' ? (unsigned long)c[i-2]-'0' : (unsigned long)c[i-2]-'A'+10);
	n1=36*n1+(i<1 ? 36 : c[i-1]>='0'&&c[i-1]<='9' ? (unsigned long)c[i-1]-'0' : (unsigned long)c[i-1]-'A'+10);
	n1=10*n1+c[i]-'0';     
	n1=27*n1+(n<1?26:(unsigned long)c[i+1]-'A');
	n1=27*n1+(n<2?26:(unsigned long)c[i+2]-'A');
	n1=27*n1+(n<3?26:(unsigned long)c[i+3]-'A');

	if(!nadd){ 
	 strupr(l); //grid square Maidenhead locator (uppercase)
	 ng=180*(179-10*((unsigned long)l[0]-'A')-((unsigned long)l[2]-'0'))+10*((unsigned long)l[1]-'A')+((unsigned long)l[3]-'0');
	}

	p=p>60?60:p<0?0:p+corr[p%10];
	n2=(ng<<7)|((unsigned long)p+64+nadd);

	// pack n1,n2,zero-tail into 50 bits
	packed[0] = n1>>20;
	packed[1] = n1>>12;
	packed[2] = n1>>4;
	packed[3] = ((n1&0x0f)<<4)|((n2>>18)&0x0f);
	packed[4] = n2>>10;
	packed[5] = n2>>2;
	packed[6] = (n2&0x03)<<6;
	packed[7] = 0;
	packed[8] = 0;
	packed[9] = 0;
	packed[10] = 0;  

	// convolutional encoding K=32, r=1/2, Layland-Lushbaugh polynomials


	for(j=0;j!=sizeof(packed);j++){ 
	  for(i=7;i>=0;i--){
		 unsigned long poly[2] = { 0xf2d05351L, 0xe4613c47L };
		 nstate = (nstate<<1) | ((packed[j]>>i)&1);
		 for(s=0;s!=2;s++){   //convolve
			unsigned long n = nstate & poly[s];
			int even = 0;  // even := parity(n)
			while(n){
			   even = 1 - even;
			   n = n & (n - 1);
			}
			symbol[k] = even;
			k++;
		 }
	  }
	}
		      

	for(i=0;i!=162;i++){
	  // j0 := bit reversed_values_smaller_than_161[i]
	  unsigned char j0=0;
	  p=-1;
	  for(k=0;p!=i;k++){
		 for(j=0;j!=8;j++)   // j0:=bit_reverse(k)
		 {

			j0 = ((k>>j)&1)|(j0<<1);

		 }
		 if(j0<162)
		   p++;
	  }
	  symbols[j0]=npr3[j0]|symbol[i]<<1; //interleave and add sync vector
	}
}
