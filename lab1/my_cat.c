#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
#include<errno.h> 

int ch;
void fatal(char * mesaj_eroare)
{
    perror(mesaj_eroare);
    _exit(-1);
}


int main(int argc,char **argv){
	int file;
	int afisat;
	char buff[1]; 
	file = open(argv[1],O_RDONLY);
	if(file < 0)
		fatal("Error opening file");
	lseek(file,0,SEEK_SET);
	while((afisat = read(file,buff,1))){
		if(afisat<0)
			fatal("Error reading char");
		ch=write(0,buff,1);
		if(ch<0)
			fatal("Error writing char");
	}
	close(file);
	return 0;
}
