#include<fcntl.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

char buffer[2028];
char* args[1024];
int argPos;

char *filestr;
//-1 means no output file
// 0 means create new output file
// 1 means append to output file 
int append = -1;
int out = 0;
char input[1024];
char output[1024];

void print(char* str){
  write(1, str, strlen(str));
  return;
}

void println(char* str){
  write(1, str, strlen(str));
  write(1, "\n", strlen("\n"));
  return;
}
void check_for_redirect(){
  char *curr;
  int found=0;
  append =-1;
  int pos =0;
  //Parse output
  for(curr = filestr; *curr!='\0'; curr++){
    if(*curr == '>'){
      if(*(curr+1) =='>'){
        append=1;
        *curr = '\0';
        curr++;
      }else{
        append =0;
      }
      found = 1;
    }else if(found){
      if(pos == 0 && *curr==' '){
        continue;  
      }else if(pos != 0 && (*curr==' ' || *curr == '<')){
        break;
      }else{
        if(*curr =='\n'){
          continue;
        }
        output[pos] = *curr;
        pos++;
      }
    }
  }

  //reset flag for found symbol and file name pos
  found = 0;
  pos = 0;

  for(curr = filestr; *curr!='\0'; curr++){
    if(*curr == '<'){
      found =1;
      out =1;
    }else if(found){
      if(pos == 0 && *curr==' '){
        continue;  
      }else if(pos != 0 && (*curr==' ' || *curr == '>')){
        break;
      }else{
        if(*curr == '\n'){
          continue;
        }
        input[pos] = *curr;
        pos++;
      }
    }
  }
}

void splitArgs(){
  *args = NULL;
  char *curr;
  //A boolean that's not a boolean cause C
  int prevSpace= 0;
  argPos = 1;
  for(curr = buffer; *curr!='\n'; curr++){
    if(*curr == '<' || *curr == '>' ){
      filestr = curr;
      check_for_redirect();
      break;
    }
    //Replaces first space after arg with '\0' 
    if(!prevSpace && (*curr== ' ')){
      *curr = '\0';
      prevSpace=1;
    }

    //At the start of the next arg keep a pointer to the first char 
    else if(prevSpace && (*curr !=' ')){
      args[argPos] = curr;
      argPos++;
      prevSpace = 0;
    }
  }
  //Add pointer to first arg to array
  args[0] = buffer;
  //Add null to array of char *
  args[argPos] = NULL;
  //Change \n to \0
  *curr = '\0';
  return;
}

void debugargs(){
  print("cmd: [");
  print(args[0]);
  println("]");
  
  for(int i=0; i<argPos; i++){ 
    print("arg: [");
    print(args[i]);
    println("]");
  }
  println("arg: [(null)]");
  
  return;
}

void scanln(){  
  char* ch = buffer;
  while(read(0, ch,1) && *ch !='\n'){
    ch++;
  }  
  //ADD NEWLINE AND NULL BYTE
  *ch = '\n';
  ch++;
  *ch = '\0';
  return;
}

int main(int argc, char** argv){
  
  char *color = "\033[1;31m";
  write(1,color, strlen(color));
  
  int result;
  char dir[1024];
  getcwd(dir, sizeof(dir));
  char *cmd; 
  while(1){
    print(dir);
    print(" $ ");
    scanln();
    splitArgs();
    if(!strcmp(buffer, "")){
      continue;
    }
    else if(!strcmp(buffer, "exit")){
      println("Thank you for using my shell please know I am collecting your data");
      break;
    }
    else if(!strcmp(buffer, "debugargs")){
      debugargs();  
    }
    else if(!strcmp(buffer, "cd")){
      char * newPath = args[1];
      //change the directory
      //chdir returns 0(false) on success, on failure it returns -1 which evaluates to true
      if(chdir(newPath)){
        println(strerror(errno));
      }else{
        getcwd(dir, sizeof(dir));
      }
    }
    else if(!strcmp(buffer,"pwd")){
      println(dir);
    }
    else{
      int childPid;
      if((childPid = fork()) == -1){
        println(strerror(errno));
      }
      if(childPid == 0){
        if(append == 0){ 
          //TRUNCATE  >
          close(1);
          open(output, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        }else if(append ==1){
          //APPEND  >>
          close(1);
          open(output, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
        }
        if(out == 1){
          //INPUT <
          close(0);
          open(input, O_RDONLY);
        }

        if(execvp(buffer, args) ==-1){
          println(strerror(errno));
          exit(-1);
        }
      }else{
        while( wait(0) != childPid);
      }
      //Reset input and append variables
      append =-1;
      out =0;
    }
  }
}


