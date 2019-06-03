double deg2rad(double deg)
{
      return (3.14 * deg / 180.0);
}



int main(void){

	int asdf;
	for (int i = 0; i < 30000; i++){
		
		__asm__("WFI");
		asdf=deg2rad(i);
	
	}

}
