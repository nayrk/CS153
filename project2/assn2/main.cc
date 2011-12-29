#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "assn2.h"

using namespace std;

int main() 
{
  cout << "Testing read mode with ls" << endl << endl;
  
  FILE * stream = Popen("ls", "r");
  char sentence[256];
  
  while(fgets(sentence, 256, stream) != NULL)
  {
    fputs(sentence, stdout);
  }
  
  Pclose(stream);
  
  cout << endl << endl << "Testing write mode with GREP" << endl << endl;

  stream = Popen("grep doit assn2.h", "w");
  
  return 0;
}
