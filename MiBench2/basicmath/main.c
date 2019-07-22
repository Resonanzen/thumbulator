#include <stdio.h>
#include "snipmath.h"
#include <math.h>
#include "../bareBench.h"

/* The printf's may be removed to isolate just the math calculations */

int main(void)
{
  double  a1 = 1.0, b1 = -10.5, c1 = 32.0, d1 = -30.0;
  double  a2 = 1.0, b2 = -4.5, c2 = 17.0, d2 = -30.0;
  double  a3 = 1.0, b3 = -3.5, c3 = 22.0, d3 = -31.0;
  double  a4 = 1.0, b4 = -13.7, c4 = 1.0, d4 = -35.0;
  double  x[3];
  double X;
  int     solutions;
  int i;
  unsigned long l = 0x3fed0169L;
  struct int_sqrt q;
  
  /* solve soem cubic functions */
 
  /* Now solve some random equations */
  for(a1=1;a1<2;a1++) {
    for(b1=10;b1>3;b1--) {
      for(c1=5;c1<6;c1+=0.5) {
	for(d1=-1;d1>-3;d1--) {
	  __asm__("WFI");
	  SolveCubic(a1, b1, c1, d1, &solutions, x);  
	  printf("Solutions:");
	  for(i=0;i<solutions;i++)
	    printf(" %f",x[i]);
	  printf("\n\r");
	}
      }
    }
  }

  printf("********* INTEGER SQR ROOTS ***********\n\r");
  /* perform some integer square roots */
  for (i = 0; i < 500; ++i)
    {

	if (i%100 == 0){
	__asm__("WFI");
}
      usqrt(i, &q);
    	// remainder differs on some machines
     // printf("sqrt(%3d) = %2d, remainder = %2d\n\r",
     printf("sqrt(%3d) = %2d\n\r",
	     i, q.sqrt);
    }
  usqrt(l, &q);
  //printf("\n\rsqrt(%lX) = %X, remainder = %X\n\r", l, q.sqrt, q.frac);
  printf("\n\rsqrt(%lX) = %X\n\r", l, q.sqrt);


  printf("********* ANGLE CONVERSION ***********\n\r");
  /* convert some rads to degrees */
	double aba;  
for (int i = 0; i < 360; i++){
   aba = deg2rad(i);
	if (i % 10 == 0){
    __asm__("WFI");
     }
 }




for (int i = 0; i < 360; i++){
   aba = rad2deg(i);
	if (i % 10 == 0){
    __asm__("WFI");
     }
}}
