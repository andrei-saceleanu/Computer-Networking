#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
#include<errno.h> 
#define MAX_LINE_LEN 8192
int ch;

void fatal(char * mesaj_eroare)
{
    perror(mesaj_eroare);
    _exit(-1);
}

void print_line(char *line,int k){
	int i;
	for(i=k-1;i>=0;i--){
		ch=write(0,&line[i],1);
		if(ch<0)
			fatal("Error writing char");
	}
}

int main(int argc,char **argv){
	int file;
	int size;
	char buff[1];
	char line[MAX_LINE_LEN]; 
	file = open(argv[1], O_RDONLY);
	if(file<0)
		fatal("Error opening file");
    size = lseek(file, -1, SEEK_END);
	int len = 0;
	int i = size;
	char *new_line = "\n";
	while(i>=0){
		ch=read(file,buff,1);
		if(ch<0)
			fatal("Error reading char");
		if(buff[0] == '\n'||i == 0){
			if(i==0){
				write(0,&buff[0],1);
				if(ch<0)
					fatal("Error writing first char in file");
			}
			print_line(line,len);
			if(line[0] != 0){
				ch=write(0,new_line,1);
				if(ch<0)
					fatal("Error writing newline");
			}
			len=0;
		}
		else{
			line[len++] = buff[0];
		}
		i--;
		lseek(file,-2,SEEK_CUR);
	}
	close(file);
	return 0;
}
