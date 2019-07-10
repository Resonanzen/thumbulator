double deg2rad(double deg)
{
      return (3.14 * deg / 180.0);
}



int main(void){

	int asdf;
	for (int i = 0; i < 30000; i++){
		if (i % 1000== 0){
		__asm__("WFI");}
		asdf=deg2rad(i);
	
	}


	for (int i = 0; i < 30000; i++){
		if (i % 1000== 0){
		__asm__("WFI");}
		asdf=deg2rad(i);

		if (i == 26909){
			break;		
		}
	
	}



	for (int i = 0; i < 30000; i++){
		if (i % 1000== 0){
		__asm__("WFI");}
		asdf=deg2rad(i);
	
	}

}
