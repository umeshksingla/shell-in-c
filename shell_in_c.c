#include <unistd.h>
#include <sys/types.h>
//#include <sys/wait.h>
#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>

uid_t getuid(void);
//uid_t geteuid(void);

char c;
struct passwd * getpwuid (uid_t uid);
int gethostname (char *a,size_t len);

char**split_line(char*line,char*delim){
		size_t size = 32;
		int pos=0;
		char*token;
		char **commands = (char**)malloc(size * sizeof(char*));
		
		if(!commands){
			fprintf(stderr,"Memory can't be allocated");
		}
		
		token = strtok(line,delim);	
		
		while(token!=NULL){
			
			commands[pos] = token;
			pos++;
			
			//printf("%s\n",token);
			token = strtok(NULL,delim);
			//printf("--------%s\n",token);
			if(pos>=size){
				commands = realloc(commands,(size+32)*sizeof(char*));
				printf("yes\n");
			}
		}
		commands[pos] = NULL;
		return commands;
}


int execute_command(char**command,int len){
		
		int status;
		if(strcmp(command[0],"exit")==0){
				printf("inside exit\n");
				status = 0;
				return 0;
		}
		
		else if(strcmp(command[0],"cd")==0){
				printf("inside cd");
				status=chdir(command[1]);
				if(!status){
					return !status;
				}
		}
		
		pid_t pid, wpid;
		int j;
		
		/*for(j=0;command[j]!=NULL;j++){
			printf("exec....%s\n",command[j]);
		}*/

		pid = fork();
		//printf("pid %d\n",pid);
		
		if (pid == 0) {
			printf("forking done%d\n",pid);
			// Child process
			status = execvp(*command, command);
			if (status<0) {
				perror(command[0]);
				_exit(0);
			}			
			_exit(1);
			
		} 
		else if(pid < 0){
			// Error forking
			printf("error pid %d\n",pid);
			perror(command[0]);
			_exit(0);
		} 
		//else {
			// Parent process
			printf("in parent%d %d\n",getpid(),pid);
			/*do{
				wpid = waitpid(pid, &status, WUNTRACED);
			} 
			while (!WIFEXITED(status) && !WIFSIGNALED(status));
			*/
			/*if(!strcmp(command[len],"&")==0)
					printf("yes\n");*/
				/*	waitpid(pid,&status,0);*/
		//}
			wait();
		return 1;
}

int main(){

		char a[1024],b[1024],c[1024];
		char*d="~";
		ssize_t size = 0;
		size_t line;
		int pos = 0;

		char*delim1=";";
		char*delim2=" \t";
		
		getcwd(c,sizeof c);
		while(1){
				//buffer = (char*)malloc(size * sizeof(char));
				/*if(!buffer){
				  perror("Unable to allocate buffer: ");
				  exit(1);
				  }*/
		gethostname(a,sizeof a);
		getcwd(b,sizeof b);
		
		int cmp = strcmp(b,c);
		if(cmp==0){
			printf("<%s@%s:%s> ",getpwuid(geteuid())->pw_name,a,d);
		}
		else if(cmp>0){
			printf("%s\n",b);	
			printf("<%s@%s:%s> ",getpwuid(geteuid())->pw_name,a,b);
		}
				
				char*buffer = (char*)malloc(sizeof(char)*1024);
				
				char c;
				int k=0;
				c=getchar();

				while(c!='\n'){
					buffer[k]=c;
					k++;
					c=getchar();
				}
				buffer[k]='\0';
				//printf("%s\n",buffer);

				char**commands={NULL};
				commands = split_line(buffer,delim1);

				int i,status,j;
				int len=-1,len1=-1;

				for(i=0;commands[i]!=NULL && commands[i]!="\n";i++){
						len++;
						//printf("%s\n",commands[i]);
						char**args;
						args = split_line(commands[i],delim2);
						for(j=0;args[j]!=NULL;j++){
								len1++;
								printf("------%s\n",args[j]);
						}
						status = execute_command(args,len1);	
						printf("status %d\n",status);
						free(args);
				}
				//free(args);
				free(commands);
		}
		return 0;
}	
