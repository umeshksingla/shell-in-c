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

uid_t getuid(void);
struct passwd * getpwuid (uid_t uid);
int gethostname (char *a,size_t len);

char cwd[2048];
pid_t current_proc;
char current_proc_name[100];
pid_t shell_id;

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

		if(signo == SIGTSTP){
				printf("i m in sigtstp\n");
				pro*temp =(pro*)malloc(sizeof(pro));
				temp->pid = current_proc;
				strcpy(temp->name, current_proc_name);
				temp->index = 0;
				temp->bg = 1;
				insert_backg(temp);
				
				kill(current_proc,SIGSTOP);
		}

		if(signo == SIGCHLD){

				union wait wstat;
				pid_t pid;

				while(1){
						pid = wait3 (&wstat, WNOHANG, (struct rusage *)NULL );
						if (pid == 0)
								return;
						else if (pid == -1)
								return;
						else {
								int u = delete_backg(pid);
								printf ("\nPid: %d\nReturn code: %d\n", pid, wstat.w_retcode);
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
		while(temp){
				if(temp->index == id){
						printf("%s\n",temp->name);
						kill(temp->pid,SIGCONT);
						wait(NULL);
						int i = delete_backg(temp->pid);
						if(i<0)
								printf("No such process\n");
						return i;
				}
				temp=temp->next;
		}
		return -5;
}

int overkill_c(){
		pro*temp = backg;
		while(temp){
				if(kill(temp->pid,SIGKILL)!=0)
						printf("Can't kill %s\tPID:[%d]\n",temp->name,temp->pid);
				else
						printf("Killed %s\n",temp->name);
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
		if(kill(temp->pid,signal)==0)
				printf("Signal sent to %s Process\n",temp->name);
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
		//printf("inside loop\n");
		
		if(strcmp((*cmd)[0],"fg")==0){	//	"fg" bring a background process to foreground

				i = fg_c(*cmd);
				return i;
		}
		else if(strcmp((*cmd)[0],"cd")==0){	//	change the directory

				i = cd_c(*cmd);
		}	
		else if(strcmp((*cmd)[0],"quit")==0){	// if the user types "exit", just return the status to be 0

				return quit_c();
		}

		while (*cmd != NULL)
		{
				pipe(p);
				if ((pid = fork()) < 0)
				{
						exit(EXIT_FAILURE);
				}
				else if (pid == 0)
				{
						//printf("nfdknk %s\n",(*cmd)[0]);
						dup2(fd_in, 0); //change the input according to the old one 
						if (*(cmd + 1) != NULL)
								dup2(p[1], 1);
						close(p[0]);

						if(strcmp((*cmd)[0],"pinfo")==0){			// "pinfo" for printing process related details and its status

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
						current_proc = pid;
						signal(SIGTSTP,sig_handler);
						strcpy(current_proc_name,(*cmd)[0]);
						status = execvp((*cmd)[0], *cmd);
						if(status<0)
								if(i<0)
									perror((*cmd)[0]);
						i=0;
						exit(EXIT_FAILURE);
				}
				else
				{
						pro*P = (pro*)malloc(1*sizeof(pro));
						int bg = check_for_bg(*cmd);
						if(!bg){	// if not background, then wait
								waitpid(pid,NULL,WUNTRACED);
						}
						else{
								if(bg){
										cmd[bg]=NULL;
								}
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

		signal(SIGINT,SIG_IGN);
		signal(SIGTSTP,SIG_IGN);

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
