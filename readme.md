README:

- This C program is a simple implementation of an interactive shell program. The shell supports semi-colon separated list of commands.
- Basic builtin commands like echo, pwd and cd have been implemented in this. Of course, the project is in progress.
- Almost all linux terminal commands can be run on this.
- 'cd' command is a bit different here. Instead of taking the default home folder of the system as '~', it takes '~' as the folder from where the shell was invoked. Running 'cd ' or 'cd ~' command will take you to this folder.
- 'pinfo pid' command prints out the state of a process and other details with process id as pid. If no pid is given as an argument, it prints the corresponding details for current process i.e. a.out.
- Background and foreground process working according to user's wish.
- 'overkill' for killing all background jobs
- 'jobs' for displaying all running background jobs
- 'kjob index signal' for sending a signal to a job with index as in jobs
- Ctrl+Z for sending a foreground process to backgorund with stopped state
- Piping and redirection works. Append to a file also works.
- Add '&' after a job to send it to background.

IN FUTURE:
- More user-interactive
- More user-defined commands.

Umesh Kumar Singla
201401204

