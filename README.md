This is a basic shell program that supports the following:

# Available commands:
--------------------

### cd
**cd**   goes to home directory

**cd -** toggles to previous directory

**cd ..[/..][/dir]** goes up and down a directory tree

**cd /dir** goes to directory dir

**cd dir** goes to subfolder dir

### alias:
**alias a=b** sets alias a=b

**alias ab="ls -a"** can escape space with double quotes

### history:
**history** will show last 5 commands (adjusted by internal constant)

**!!** history replacement with previous command

**!-1** same as **!!**

**!-n** history replacement with n-commands ago

**!n** history replacement with command #n

### exit:
**exit** exits shell
### jobs:
**jobs** will show currently running background jobs (if any)
### redirection:
**command < input.txt** redirects stdin to use input.txt

**command < "lorem ipsum"** double quotes can be used to escape spaces

**command > file** redirects stdout to file

**command 2> file** redirects stderr to file

**command &> file** redirects both stdout and stderr to file

**command &> /dev/null** mutes stdout and stderr
### source:
**source [file]** reads aliases from file. If file is not specified, defaults to reading from .woosh/woosh.rc
### background:
**command &** will execute command as background process
### other commands:
**ls -a** will fork a child process to execute **ls** with **-a** as part of argument list

