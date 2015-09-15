#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <fcntl.h>
uid_t getuid(void);

char cwd[2048];
struct passwd * getpwuid (uid_t uid);
int gethostname (char *a,size_t len);

void print_prompt(){

		char a[1024],b[1024],e[1024];
		char*d="~";
		gethostname(a,sizeof a);								//get pc name
		getcwd(b,sizeof b);										//get current directory

		int cmp = strcmp(b,cwd);			
		if(cmp==0){												//if same as you started with, then place ~			
				printf("<%s@%s:%s> ",getpwuid(getuid())->pw_name,a,d);
		}

		else if(cmp>0){											//if bigger, then append to e, whatever there is after ~	
				printf("%s\n",b);	
				int p=0,q;
				e[p]=d[0];
				p++;
				for(q=strlen(cwd);q<strlen(b);q++){
						//	printf("%c\t",b[q]);
						e[p]=b[q];
						p++;
				}
				e[p++]='\0';
				printf("<%s@%s:%s> ",getpwuid(getuid())->pw_name,a,e);
		}

		else if (cmp<0){										//if smaller, then print the location as it is.
				printf("<%s@%s:%s> ",getpwuid(getuid())->pw_name,a,b);
		}
}

void sig_handler(int signo){
		if(signo == SIGINT){
				//print_prompt();
				printf("\n");
				print_prompt();
				fflush(stdout);
		}
}
char**split_line(char*line,char*delim){				
		/*splits the command line according to the delim and returns the array of individual strings*/
		size_t size = 32;
		int pos=0;
		char*token;
		char **commands = (char**)malloc(size * sizeof(char*));


		if(!commands){
				fprintf(stderr,"Memory can't be allocated to commands.");	//if enough memory is not available for allocating to the commands
		}

		token = strtok(line,delim);	

		while(token!=NULL){

				commands[pos] = token;
				pos++;

				//printf("%s\n",token);
				token = strtok(NULL,delim);
				//printf("--------%s\n",token);

				if(pos>=size){	// if pos becomes >= size, we need to allocate more memory for further commands
						size+=size;
						commands = realloc(commands,(size)*sizeof(char*));
						//printf("yes\n");
				}
		}
		commands[pos] = NULL;	//last ensured to be NULL
		return commands;
}

int quit_c(){
		return 10;
}

int execute_command(char**command){
		/*executes the command and takes whole command as an input*/

		int status;
		pid_t pid1,pid2;

		if(strcmp(command[0],"quit")==0){	// if the user types "exit", just return the status to be 0
				//printf("inside exit\n");
				status = quit_c();
				return status;
		}	
		else if(strcmp(command[0],"cd")==0){	//	if the user types "cd ..."
				//printf("inside cd\n");

				if(!command[1]){
						status=chdir(cwd);
				}
				else if (strcmp(command[1],"~")==0){
						status=chdir(cwd);
				}
				else if(command[1][0]=='~' && command[1][1]!='\0'){
						char l[2048];
						//int len=0;
						strcpy(l,cwd);
						command[1]++;
						strcat(l,command[1]);
						status=chdir(l);
				}
				else{
						status=chdir(command[1]);
				}
				return status;
		}
		else if(strcmp(command[0],"pinfo")==0){			// "pinfo" for printing process related details and its status

				char p[50];

				if(!command[1]){						//if no process id is given, we print out info for current process i.e. a.out
						sprintf(p,"%d",getpid());
				}
				else{
						sprintf(p,"%s",command[1]);
				}

				char**arg;
				char r[100];
				arg=(char**)malloc(sizeof(char*)*1024);

				if(!arg){
						fprintf(stderr,"Memory can't be allocated to arg.");	//if enough memory is not available for allocating to the commands
				}

				arg[0]="cat";
				strcpy(r,"/proc/");
				strcat(r,p);
				strcat(r,"/status");
				arg[1]=r;

				pid1 = fork();
				wait(&status);

				if (pid1 == 0) {
						status = execvp(*arg, arg);
						if (status<0) {
								perror("pinfo: ERROR while executing pinfo.");
								_exit(0);
						}			
						_exit(1);

				} 
				else if(pid1 < 0){
						perror("pinfo: can't execute.");
						_exit(0);
				} 

				return 1;
		}


		else{

				//for system commands
				pid2 = fork();		// divide into two

				//printf("reached here.2\n");
				//printf("pid %d\n",pid);
				wait(&status);
				if (pid2 == 0) {
						//printf("forking done%d\n",pid);
						// Child process
						int input,output,i;

						for(i=0;command[i];i++){
								//printf("printing cmd '%s' at %d \n",command[i],i);
								if(strcmp(command[i],"<")==0){	// input is being redirected from some file
										command[i]=NULL;
										if(command[i+1]){
												input=open(command[i+1],O_RDONLY);
												dup2(input,0);
										}
										else
												fprintf(stderr,"No input file specified with <");										
								}

								if(strcmp(command[i],">")==0){	//	output is being redirected to some file
										command[i]=NULL;
										if(command[i+1]){
												output=open(command[i+1],O_RDONLY | O_WRONLY | O_CREAT, S_IRWXU);
												dup2(output,1);
										}
										else
												fprintf(stderr,"No output file specified with >");
								}
																
						}
						
						close(input);
						close(output);

						status = execvp(*command, command);

						if (status<0) {
								perror(command[0]);
								_exit(0);
						}			
						_exit(1);

				} 
				else if(pid2 < 0){
						// Error forking
						//printf("error pid %d\n",pid);
						perror(command[0]);
						_exit(0);
				} 

				return 1;
		}
}



int main(){

		//char a[1024],b[1024],e[1024];
		//char*d="~";
		ssize_t size = 0;
		size_t line;
		int pos = 0;

		char*delim1=";";
		char*delim2=" \t";

		getcwd(cwd,sizeof cwd);											//get the directory name from where the shell is invoked

		if(signal(SIGINT,sig_handler)==SIG_ERR)
				printf("\ncan't catch ctrl c\n");

		while(1){

				print_prompt();
				char*buffer = (char*)malloc(sizeof(char)*2048);

				char m;
				int k=0;

				m=getchar();
				while(m!='\n'){											//scanning till we get '\n'
						buffer[k]=m;
						k++;
						m=getchar();
				}
				buffer[k]='\0';
				//printf("%s\n",buffer);

				char**commands={NULL};
				commands = split_line(buffer,delim1);					//splitting the line scanned by ;

				int i,status,j;
				int len=-1,len1=-1;

				for(i=0;commands[i]!=NULL && commands[i][0]!='\n';i++){	//iterating over each command along with its arguments
						len++;
						//printf("%s\n",commands[i]);
						char**args;
						args = split_line(commands[i],delim2);			//splitting a command by " "
						if(args[0]!=NULL)
								status = execute_command(args);					//executing the command

						//printf("status = %d\n",status);				

						if(status==10){
								return 0;									//exiting the shell
						}
						if(status<0){
								printf("No such file or directory.\n");
						}
						free(args);										//freeing the args array
				}
				free(buffer);	//freeing the buffer line					
				free(commands);	//freeing the commands array

				printf("\n");
		}
		return 0;
}	
