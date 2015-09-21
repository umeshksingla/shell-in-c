/*----------created by umeshksingla on 15/09/2015-----------------*/

//standard libraries
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
#include <sys/resource.h>
#include <ctype.h>

uid_t getuid(void);
struct passwd * getpwuid (uid_t uid);
int gethostname (char *a,size_t len);

char cwd[2048];
pid_t current_proc;
char current_proc_name[100];
pid_t shell_id;

pid_t shell_pgid;			//shell's process group id
int interactive, shell_terminal, back, redirection;
pid_t child_pid;			//pid of the foreground running process (0 if shell is in foreground)

struct pro{
		int index;
		pid_t pid;
		int bg;
		char name[100];
		struct pro*next;
		//char args[100][100];
};

typedef struct pro pro;

pro*backg=NULL;

int backl = 1;


void insert_backg(pro*P){

		if(backg==NULL){
				P->index = backl++;
				backg = P;
				return;
		}
		P->index = backl++;
		P->next=backg;
		backg=P;
		return;
}

int delete_backg(pid_t pid){
		pro*temp=backg;
		pro*p;
		if(temp && temp->pid == pid){
				backg = temp->next;
				free(temp);
				return 5;
		}
		while(temp && temp->pid != pid){
				p=temp;
				temp=temp->next;
		}
		if(!temp)
				return -5;
		p->next=temp->next;
		free(temp);
}

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
		int i,j;
		if(signo==SIGINT){		
				printf("\n");
				print_prompt();
				fflush(stdout);
		}	

		else if(signo == SIGTSTP){
				//nothing
		}

		else if(signo == SIGCHLD){
				int status;
				pid_t pid;
				while((pid=waitpid(-1,&status,WNOHANG))>0){                    //return if no child has changed state
						if(pid>0){
								if(WIFEXITED(status)){		
										pro*temp = backg;		
										while(temp){
												if(temp->pid==pid){
														break;
												}
												temp=temp->next;
										}
										if(temp!=NULL){
												fprintf(stdout,"\n%s with pid %d exited normally\n",temp->name,pid);
												i = delete_backg(pid);
										}
								}
								else if (WIFSIGNALED(status)) { 
										pro*temp = backg;		
										while(temp){
												if(temp->pid==pid){
														break;
												}
												temp=temp->next;
										}
										if(temp!=NULL){
												fprintf(stdout,"\n%s with pid %d signalled to exit\n",temp->name,pid);
												j = delete_backg(pid);
										}
								}
						}
				}
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

				token = strtok(NULL,delim);

				if(pos>=size){	// if pos becomes >= size, we need to allocate more memory for further commands
						size+=size;
						commands = realloc(commands,(size)*sizeof(char*));
				}
		}
		commands[pos] = NULL;	//last ensured to be NULL
		return commands;
}

int quit_c(){
		return 10;
}

int cd_c(char**command){

		int status;
		char l[2048];
		if(!command[1]){
				status=chdir(cwd);
		}
		else if (strcmp(command[1],"~")==0){
				status=chdir(cwd);
		}
		else if(command[1][0]=='~' && command[1][1]!='\0'){
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

int pinfo_c(char**command){

		char p[100],r[100];
		char**arg;
		int status;
		pid_t pid1;

		if(!command[1])						//if no process id is given, we print out info for current process i.e. a.out
				sprintf(p,"%d",getpid());
		else
				sprintf(p,"%s",command[1]);

		arg=(char**)malloc(sizeof(char*)*1024);

		if(!arg)
				fprintf(stderr,"Memory can't be allocated to arg.");	//if enough memory is not available for allocating to the commands

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
						return -5;
				}			
				return 1;
		} 
}

int jobs_c(){
		pro*temp=backg;
		if(!temp)
				printf("No background processes running\n");
		while(temp){
				printf("[%d]\t%s\tPID:[%d]\n",temp->index,temp->name,temp->pid);
				temp=temp->next;
		}
		return 1;
}

int fg_c(char**command){

		int id;
		if(command[1])
				id = atoi(command[1]);
		else{
				printf("No argument passed to fg\n");
				return -1;
		}
		pro *temp=backg;
		pid_t pgid;
		int status,i;
		while(temp){
				if(temp->index == id){
						printf("%s\n",temp->name);
						pgid=getpgid(temp->pid);
						temp->bg=0;
						tcsetpgrp(shell_terminal,pgid);  // give control of terminal to the process from our shell
						child_pid=pgid;	
						if(killpg(pgid,SIGCONT)<0)		//send a SIGCONT signal to the process group
								perror("Not able to bring to foreground");
						waitpid(temp->pid,&status,WUNTRACED);		// wait for it to be stopped or terminated
						if(!WIFSTOPPED(status)){
								child_pid=0;
								i = delete_backg(temp->pid);	// if terminated remove from jobs table
								if(i<0)
										printf("No such process\n");
						}
						else {
								printf("\n%s stopped\n",temp->name);	
						}
						tcsetpgrp(shell_terminal,shell_pgid); // return control of terminal to our shell
						return i;
				}
				temp=temp->next;
		}
		return -5;
}

int overkill_c(){
		pro*temp = backg;
		while(temp){
				if(killpg(getpgid(temp->pid),SIGKILL)!=0)
						printf("Can't kill %s\tPID:[%d]\n",temp->name,temp->pid);
				temp=temp->next;
		}
		return 1;
}

int kjob_c(char**command){

		if(!command[1]){
				printf("Process ID required\n");
				return 1;
		}
		if(!command[2]){
				printf("Signal not specified.\n");
				return 1;
		}

		int index = atoi(command[1]);
		int signal = atoi(command[2]);
		pid_t pid,pgid;
		pro*temp=backg;

		while(temp){
				if(temp->index == index){
						break;
				}
				temp=temp->next;
		}
		if(!temp){
				printf("No such Process.\n");
				return 1;
		}
		if(killpg(getpgid(temp->pid),signal)==0){
				if(signal==9){
						int j = delete_backg(temp->pid);				
				}
				printf("Signal sent to %s Process\n",temp->name);
		}
		else	
				printf("Signal can't be sent. Some Error Occured.\n");
		return 1;
}

int check_for_bg(char**command){
		int i;
		for(i=0;command[i];i++){
				if(strcmp(command[i],"&")==0 && command[i+1]==NULL) {
						command[i]=NULL;
						return i;	// it's background, so true
				}
		}
		return 0;	// it's not in background, so false
}

int loop_pipe(char ***cmd) 
{
		int p[2];
		pid_t pid;
		int fd_in = 0;
		int i, status;

		signal(SIGCHLD,sig_handler);
		signal(SIGTSTP,sig_handler);
		//printf("inside loop\n");

		if(strcmp((*cmd)[0],"fg")==0){	//	"fg" bring a background process to foreground

				return fg_c(*cmd);
		}
		else if(strcmp((*cmd)[0],"cd")==0){	//	change the directory

				return cd_c(*cmd);
		}	
		else if(strcmp((*cmd)[0],"quit")==0){	// if the user types "exit", just return the status to be 0

				return quit_c();
		}
		else if(strcmp((*cmd)[0],"pinfo")==0){	// "pinfo" for printing process related details and its status

				i = pinfo_c(*cmd);
		}

		else if(strcmp((*cmd)[0],"jobs")==0){		//	"jobs" for printing currently running jobsi

				i = jobs_c();
		}

		else if(strcmp((*cmd)[0],"overkill")==0){	//	"overkill" kills al background process at once

				i = overkill_c();
		}
		else if(strcmp((*cmd)[0],"kjob")==0){	//	"kjob" sends a signal to a particular process

				i = kjob_c(*cmd);
		}

		while (*cmd != NULL)
		{
				pipe(p);
				pid = fork();
				if (pid < 0)
						perror("Child process not created");
				else if (pid == 0)
				{
						dup2(fd_in, 0);	 	//	change the input according to the old one 
						if (*(cmd + 1) != NULL)
								dup2(p[1], 1);
						close(p[0]);

						setpgid(getpid(),getpid()); 

						int input,output,q;
						for(q=0;(*cmd)[q]!=NULL;q++){
								if(strcmp((*cmd)[q],"<")==0){  // input is being redirected from some file
										(*cmd)[q]=NULL;
										if((*cmd)[q+1]){
												input=open((*cmd)[q+1],O_RDONLY,S_IRWXU);
												dup2(input,0);
												close(input);
										}
										else
												fprintf(stderr,"No input file specified with <");        
								}

								else if(strcmp((*cmd)[q],">")==0){ //  output is being redirected to some file
										(*cmd)[q]=NULL;
										if((*cmd)[q+1]){
												output=open((*cmd)[q+1],O_TRUNC | O_RDONLY | O_WRONLY | O_CREAT, S_IRWXU);
												dup2(output,1);
												close(output);
										}
										else
												fprintf(stderr,"No output file specified with >");
								}

								else if(strcmp((*cmd)[q],">>")==0){ //  output is being redirected to some file
										(*cmd)[q]=NULL;
										if((*cmd)[q+1]){
												output=open((*cmd)[q+1],O_APPEND | O_RDONLY | O_WRONLY | O_CREAT, S_IRWXU);
												dup2(output,1);
												close(output);
										}
										else
												fprintf(stderr,"No output file specified with >");
								}
						}

						int bg = check_for_bg(*cmd);
						if(bg){
								cmd[bg]=NULL;		
						}
						else { 
								tcsetpgrp(shell_terminal,getpid());
								/*make the signals for the child as default*/
								signal (SIGINT, SIG_DFL);
								signal (SIGQUIT, SIG_DFL);
								signal (SIGTSTP, SIG_DFL);
								signal (SIGTTIN, SIG_DFL);
								signal (SIGTTOU, SIG_DFL);
								signal (SIGCHLD, SIG_DFL);						
						}
						current_proc = pid;
						//signal(SIGTSTP,sig_handler);
						strcpy(current_proc_name,(*cmd)[0]);
						status = execvp((*cmd)[0], *cmd);
						if(status<0)
								if(i<0){
										perror((*cmd)[0]);
										return -1;
								}
						i=-1;
						exit(EXIT_FAILURE);
				}
				else
				{
						pro*P = (pro*)malloc(1*sizeof(pro));
						int bg = check_for_bg(*cmd);
						if(!bg){	// if not background, then wait

								tcsetpgrp(shell_terminal,pid);	// to avoid racing conditions

								int status;
								child_pid=pid;
								waitpid(pid,&status,WUNTRACED);	// wait for child till it terminates or stops
								if(WIFSTOPPED(status)){
										P->pid = pid;
										strcpy(P->name,(*cmd)[0]);
										P->bg = 1;
										P->next = NULL;
										P->index = 0;
										insert_backg(P);
										printf("\n%s stopped\n",(*cmd)[0]);
								}
								tcsetpgrp(shell_terminal,shell_pgid);	
						}
						else{

								cmd[bg]=NULL;
								P->pid = pid;
								strcpy(P->name,(*cmd)[0]);
								P->bg = 1;
								P->next = NULL;
								P->index = 0;
								insert_backg(P);
						}

						close(p[1]);
						fd_in = p[0]; //save the input for the next command
						cmd++;
				}
		}
		return i;
}

int main(){
		shell_id = getpid();
		ssize_t size = 0;
		size_t line;
		int pos = 0;

		char*delim1=";";
		char*delim2=" \t";
		char*delim3="|";
		getcwd(cwd,sizeof cwd);											//get the directory name from where the shell is invoked
		shell_terminal=STDERR_FILENO;			

		interactive=isatty(shell_terminal);
		if(interactive)
		{
				while(tcgetpgrp(shell_terminal) != (shell_pgid=getpgrp()))
						kill(- shell_pgid,SIGTTIN);
		}
		signal (SIGINT, SIG_IGN);
		signal (SIGTSTP, SIG_IGN);
		signal (SIGQUIT, SIG_IGN);
		signal (SIGTTIN, SIG_IGN);
		signal (SIGTTOU, SIG_IGN);
		shell_pgid=getpid();

		if(setpgid(shell_pgid,shell_pgid)<0)
		{
				perror("not able to make shell a member of its own process group.");
				_exit(1);
		}
		tcsetpgrp(shell_terminal,shell_pgid);			//gives terminal access to the shell only

		while(1){
				if(signal(SIGINT,sig_handler)==SIG_ERR)
						perror("Signal not caught!!");
				if(signal(SIGCHLD,sig_handler)==SIG_ERR)
						perror("signal not caught!!");
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
				int v = 0,fl=1;
				char g;
				while(buffer[v]){
						g = buffer[v];
						if(isspace(g)){
								fl=1;
						}
						else
								fl=0;
						v++;
				}
				if(fl==1){
					continue;
				}
				char**commands={NULL};
				commands = split_line(buffer,delim1);					//splitting the line scanned by ;
				int i,status,j;
				int len=-1,len1=-1;

				for(i=0;commands[i]!=NULL && commands[i][0]!='\n';i++){	//iterating over each command along with its arguments
						len++;
						char **cmd[100];
						int k=0;
						char**args;
					
						args=split_line(commands[i],"|");

						int s=0;
						for(s=0;args[s];s++){
								char**each;
								each = split_line(args[s]," ");
								cmd[k++] = each;
								
						}
						cmd[k++]=NULL;
						status = loop_pipe(cmd);
						if(status==10){
								return 0;									//exiting the shell
						}
						if(status<0){	
								printf("Error!\n");
						}
				}
				free(buffer);	//freeing the buffer line					
				free(commands);	//freeing the commands array
		}
		return 0;
}	
