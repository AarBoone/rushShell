This was a project for my Operating Systems class to create a shell that can execute UNIX commands.

It just takes a string and uses some simple parsing rules to split it into commands and uses execv() to execute.

Has built in commands:
"path": path /bin /xd /yz - sets the terminals path to contain /bin, /xd, /yz
"cd": cd Folder - changes the working directory to Folder
"lp": lp - prints every element in the path (would print /bin /xd /yz after running the above path command)
"exit": exit - terminates the shell

Can execute single commands with arguments ex: ls -la
Can redirect terminal output to a different file using ">" ex: ls > files.txt
Can execute multiple commands in parallel (non-deterministically) using "&" to separate multiple commands ex: ls & ls
Includes the ability to download linux packages and run them ex: sudo apt-get install cmatrix
