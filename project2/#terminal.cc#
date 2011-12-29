#include<string>
#include<iostream>
#include<sys/wait.h>
#include<errno.h>
#include<vector>
#include<sstream>
#include<cassert>
using namespace std;
#define each(I) for( typeof((I).begin()) it=(I).begin(); it!=(I).end(); ++it )


int doit( const string progname, vector<string> tok ) {  
  // Executes a parsed command line returning command's exit status.
  if ( ! tok.size() || tok[0] == "" ) return 0;    // nothing to be done.

  if ( string(tok[0]) == "cd" ) {             // A child can't cd for us. 
    chdir( tok.size() > 1 ? tok[1].c_str() : getenv("HOME") ); 
    if ( errno ) cerr << progname << ": cd: " << strerror(errno) << endl;
    return errno;                    
  } 

  // fork.  And, wait if child to run in foregrounded
  if ( pid_t kidpid = fork() ) {       
    // You're the parent.
    int status = errno;
    if ( !status && tok.back() != "&" ) waitpid( kidpid, &status, 0 ); 
    return status;
  } 
  // You're the child.

  // Option processing: redirect I/O as requested
  char* arglist[ 1 + tok.size() ];  
  for ( int i = 0; i != tok.size(); ++i ) {
    if      ( tok[i] == "&"  ) break;
    else if ( tok[i] == "<"  ) freopen( tok[++i].c_str(), "r", stdin  );
    else if ( tok[i] == ">"  ) freopen( tok[++i].c_str(), "w", stdout );
    else if ( tok[i] == ">>" ) freopen( tok[++i].c_str(), "a", stdout );
    else if ( tok[i] == "2>" ) freopen( tok[++i].c_str(), "w", stderr );
    else if ( tok[i] == "|"  ) {
      int mypipe[2];
      pipe( mypipe );  // finds two ports and creates a pipe between.
      if ( fork() ) {          // you're the consumer (i.e., parent).
        close( mypipe[1] );       // close the write end of the pipe.
        dup2( mypipe[0], 0 );           // dup the read end to stdin.
        while ( *tok.begin() != "|" ) tok.erase( tok.begin() );
	       tok.erase( tok.begin() );                    // scan past '|' 
	       //tok.pop_front();                    // scan past '|' 
        exit( doit( progname, tok ) );    // recurse on stripped tok.
      } else {                  // you're the producer (i.e., child).
        close( mypipe[0] );        // close the read end of the pipe.
        dup2( mypipe[1], 1 );       // dup the write end onto stdout.
      }
      break;                        // exec with the current arglist.
    } else {           // add this token a C-style argv for execvp().
      arglist[i] = const_cast<char*>( tok[i].c_str() );
      arglist[i+1] = 0;     // C-lists of strings end w null pointer.
    }
  }
  
  // execute the command
  string command = (tok[0][0] == '~' ? getenv("HOME")+tok[0].substr(1) : tok[0]);
  execvp( command.c_str(), arglist );
  // if we get here, there is an error in the child's attempt to exec.
  cerr << progname << ": " << command << ": " << strerror(errno) << endl;
  exit(errno); 
}


int main( int argc, char* argv[], char* envp[] ) {
  while ( ! cin.eof() ) {
    cout << "? " ;
    string temp;
    getline( cin, temp );
    if ( temp == "exit" ) break;
    vector<string> v;
    stringstream ss(temp);  // split temp (at white space) into v
    while ( ss ) {
      string s;
      while ( ss >> s ) {
        if ( s != ";" ) v.push_back(s);
        if ( s == ";" || s == "&" ) break; 
      }
      doit( argv[0], v );
      v.clear();
    }
  }
  cout << "exit" << endl;
  return 0;
}



///////////////// Diagnostic Tools /////////////////////////

   // cout.flush();
   // if ( WIFEXITED(status) ) { // reports exit status of proc.
   //   cout << "exit status = " << WEXITSTATUS(status) << endl;
   // }

