// Aaron Boone U64069975
// The program creates and executes a rush shell
// The main function grabs a line from the user and separates that line into tokens separated by "&"
// each of these tokens is fed into the execute_command function where it is further tokenized and
// operations are performed to make it execute the expected commands

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>  // used for the isspace() function


void printError(){ // prints the standard error defined by the project
    char error_message[24] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}


// The execute function handles all built-in commands as well as all commands to be executed from path
// takes a string of "command arg arg arg" where the redirection symbol would be an argument
// "command arg > file"
// as well as an array path and the size of said array
// need to pass the path size so that if the command is the "path" function it can modify the global path size variable
void execute_command(char* token, char* path[], int *path_size) {

    // manage commands that are only whitespace
    int whitespaceFlag = 0;
    for(int i = 0; token[i] != '\0'; i++){ // Goes through token and detects in any character is not whitespace

        // This loop will always break after finding the first non-whitespace character
        // This allows us to simultaneously check if the first character is the redirection symbol
        if(!(isspace(token[i]))){ // if the given character is not whitespace
            whitespaceFlag++;    // inc Flag
            if(token[i] == '>'){ // if the first non-space character is the redirect token (redirect without command)
                printError();    // Error
                return;          // and exit the execute command function back to the shell
            }
            break;  // break from the loop because we dont care how many non-whitespace characters there are as long as there is at least one
        }
    }

        if (whitespaceFlag == 0){ // if there are no characters other than whitespace return
            return; // back to the shell with no error
        }

    char * command_exec[100];
    char delim[3] = " \t"; // this array gets passed to strsep which allows it to parse for tabs and spaces at the same time
    int numArgs_cmd = 0;   // keeps track of number of tokens (command + arguments)
    int redirIndex = 0;    // index of redirection token

    while((command_exec[numArgs_cmd] = strsep(&token, delim)) != NULL) { // Breaks the string into a new array based on whitespace

            if(!(strcmp(command_exec[numArgs_cmd],"\0"))){continue;}    // if the token is empty ignore it (dont increment number of arguments)

            else if(!(strcmp(command_exec[numArgs_cmd], ">"))){ // if the token is the redirection token
                if (redirIndex == 0){   // check if there was already a redirection token in the string
                    redirIndex = numArgs_cmd;   // if not store the index of the redirection
                    numArgs_cmd++;              // inc number of arguments
                }
                else{                   // if there was already a redirection token
                    printError();           // print an error
                    return;                 // return back to the shell
                }
            }
            else{numArgs_cmd++;}        // if not redirect or \0 increment number of arguments
        }


        command_exec[numArgs_cmd] = NULL; // NULL terminate commands array


        // from here command_exec is an array ["cmd", "arg1", "arg2", NULL]


        // Exit
        if(!(strcmp(command_exec[0], "exit"))){ // if the command is "exit"
            //If there is more than 1 element in command_exec then print error
            if(numArgs_cmd != 1){
                printError();
            }

            // Else call exit(0)
            else{
                exit(0);
            }
        }

        else if(!(strcmp(command_exec[0], "cd"))){ // if the command is "cd"
            if (numArgs_cmd == 1 || numArgs_cmd > 2){ // if there is more than 1 argument following cd error or no argument
                printError(); // print error
            }
            else{
                if(chdir(command_exec[1]) != 0){ // else change to new directory
                    printError();           // if chdir fails print an error
                }
            }
        }

        else if(!(strcmp(command_exec[0], "path"))){ // if the first token is "path"
            if (numArgs_cmd == 1){ // if path is the only token set the first element of path[] to NULL
                path[0] = NULL;
                *path_size = 0; // set the path size to 0
            }
            else{ // "path" is followed by arguements
                for(int i = 0; i<numArgs_cmd;i++){
                    path[i] = command_exec[i+1]; // fill the path[] with all the arguments following "path"
                }
                path[numArgs_cmd] = NULL;       // NULL terminate the path[]
                *path_size = numArgs_cmd - 1;   // modify global path_size variable (numArgs - 1 because the first one is "path")
            }
        }

        else if(!(strcmp(command_exec[0], "lp"))){ // This is a built-in that I added to help troubleshoot the path
            for(int i = 0; i < *path_size; i++){printf("Path %d: %s\n", i, path[i]);} // It just iterates through the path array and prints it
        }



        else{ // if its not a built in command fork and execute
            pid_t pid = fork();

            if (pid < 0) {
                // Fork failed
                printError();
                exit(-1);
            } else if (pid == 0) {
                // Child process
                char temp_path[35]; // create a string that will store the path to test
                for(int i = 0; i < *path_size; i++){
                    strcpy(temp_path, path[i]); //Add test path argument to temp path
                    strcat(temp_path, "/"); //Adds "/" to end of argument
                    strcat(temp_path, command_exec[0]); //Add command to end of path
                    int access_status = access(temp_path, X_OK);    //Check if executable within directory
                    if (access_status == 0){    // if command is in path execute

                        if(redirIndex != 0 && (numArgs_cmd - redirIndex) == 2){ // If > exists and is the 2nd to last argument redirect

                            // Redirect stdout to the last argument which should be a file descriptor
                            freopen(command_exec[numArgs_cmd -1], "w+", stdout);

                            // Remove the last 2 arguments since they are not necessary
                            // now that we have used them for redirection
                            numArgs_cmd = numArgs_cmd - 2;
                            command_exec[numArgs_cmd] = NULL;
                            command_exec[numArgs_cmd + 1] = NULL;
                        }
                        // else if > exists, error out becuase it is not the 2nd to last argument (Either too many args or not enough args after)
                        else if(redirIndex != 0){
                            // print error and exit
                            printError();
                            exit(1);
                        }

                        // At this point command has been found in path and redirection checks have happened
                        // Execute the temp path command with the args from command_exec array
                        if (execv(temp_path, command_exec) == -1) {
                            // If execvp fails print error and exit
                            printError();
                            exit(1);
                        }
                    }
                }
                // if youve reached this point the command was not found in any of the paths
                printError();
                exit(1); // we need to kill the child process still
            } else {
                // Parent Process
                // We just want to get back to the loop where we print rush> again
                return;
            }
        }

}




int main(int argc, char *argv[]){
    if(argc > 1){ // if ./rush is called with another argument error out
        printError();
        exit(1);
    }

    //Things to read lines
    size_t b = 100;
    char *src = NULL;
    char * command_line[100];

    char * path[35];
    int path_size = 1;
    path[0] = "/bin";



    while(1){
        printf("rush> "); // print
        fflush(stdout);   // project said to do this
        getline(&src, &b, stdin);
        char *buffer = strdup(src);     // at one point i was having issues with strsep() and copying the src to a buffer was a troubleshooting step that i never undid

        int num_commands = 0; // Used to count the number of commands that need to be run
        //Remove trailing newline character of input string
        buffer[strcspn(buffer, "\n")] = '\0';

        // Will tokenize string and put it into an array of strings for use in execution
        while((command_line[num_commands] = strsep(&buffer, "&")) != NULL) {
            //Each break counted then returned to reference number of command/arguments
            if(!(strcmp(command_line[num_commands],"\0"))){continue;} // if the command line is empty do nothing
            else{num_commands++;} // else increment the number of commands
        }

        // This is going to be a list of commands and arguments in the form
        // ["command1 arg arg arg", "command2 arg arg > arg", NULL] and num_commands will store the number of commands
        // in this case 2
        command_line[num_commands] = NULL; // NULL terminate the command_line array just to be safe

        for(int i = 0; i<num_commands;i++){
            // for every argument execute it using my custom execute function
            execute_command(command_line[i], path, &path_size); // takes a full command as token, a path array, and the length of the path array
        }


        // waits for all child processes to finish
        while(waitpid(-1, NULL, 0) > 0);   // this is equivalent to wait(NULL)
    }
    free(src); // truly no idea if I need this. I added it right when I started the project and it seems like bad luck to delete it
}
