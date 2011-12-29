#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace std;

void report()
{
  string line;

  //============== getting cpu info ====================//
  ifstream cpuinfo("/proc/cpuinfo");

  //checking if its open
  if(!cpuinfo.is_open())
    cerr << "failed to open file";

  //cycle through the lines until it hits the query
  while(!cpuinfo.eof() && line.find("model"))
  {
    getline(cpuinfo, line);
  }
  
  getline(cpuinfo, line);
  cout << "cpu type and model: ";
  
  //removing unnecessary information
  int pos = line.find(':') + 2;
  string x = line.substr(pos);
  cout << x;

  //closing the file
  cpuinfo.close();

  //============= getting kernel version ===============//
  ifstream version("/proc/version");
  
  //checking if its open
  if(!version.is_open())
    cerr << "failed to open uptime";

  //looking for the linux version by cycling through the
  //lines in the document
  while(!version.eof())
  {
    getline(version, line);

    if(line.find("linux") == 0)
    {
      //getting only the information we want to display
      line = line.substr(0, 34);
      cout << "\n\nlinux kernel version: " << line << endl;
    }
  }

  //closing the file
  version.close();


  //============ getting system boot time ===============//
  ifstream uptime("/proc/uptime");

  //checking if its open
  if(!uptime.is_open())
    cerr << "failed to open uptime";
  double date;
  uptime >> date;

  //converting unix time to appropriate output
  int seconds = (int)date;
  int realseconds = seconds % 60;
  int minutes = seconds / 60 % 60;
  int hours = seconds / 3600 % 24;
  int days = seconds / 86400;
	
  cout << "\namount of time since the system was last booted: ";
  cout << days << ':' << hours << ':' << minutes << ':' << realseconds << endl;

  //closing the file
  uptime.close();
    
  
  //============== getting cpu stats ================//
  ifstream proc("/proc/stat");

  //checking if its open
  if(!proc.is_open())
    cerr << "failed to open stats";
  
  //getting info
  string um, um2, um3;
  proc >> um >> um;
  proc >> um2 >> um2;
  proc >> um3;
  cout << "\ncpu time in user mode, in system mode, and "
    "idle (seconds): ";
  cout << um << ' ' << um2 << ' ' << um3 << endl;

  //closing the file
  proc.close();
    

  //============== getting disk stats =============//
  ifstream disk("/proc/diskstats");

  //closing the files
  if(!disk.is_open())
    cerr << "failed to open disk stats";
  string a,b;
  long io, io2;
  int count = 0;

  //getting the appropriate line
  while(count < 16)
  {
    getline(disk, line);
    count++;
  }
	
  disk >> a >> a >> a  >> io;
  disk >> b >> b >> b >> io2;
  cout << "\ndisk_io: " << io + io2 << endl;
 
  //closing the file
  disk.close();

  //============= getting switches/prcesses ========/
  ifstream processes("/proc/stat");

  //checking if its open
  if(!processes.is_open())
    cerr << "failed to open stat for processes";

  //cycling through to find the search query
  while(!processes.eof() && line.find("ctxt"))
  {
    processes >> line;
  }

  string p;
  processes >> p;
  cout << "\nnumber of context switches: ";
  cout << p << endl;

  processes >> p >> p >> p >> p;
  cout << "\nnumber of processes being created: ";
  cout << p << endl;

  //close the file
  processes.close();


  //============== getting memory total ============/
  ifstream mem("/proc/meminfo");

  //checking if its open
  if(!mem.is_open())
    cerr << "failed to open memory info";

  //grabbing info
  string memory;
  mem >> memory >> memory;
  cout << "\nmemtotal: " << memory << " kb" << endl;

  mem >> memory >> memory >> memory;
  cout << "\nmemfree: " << memory << " kb" << endl;

  //closing the file
  mem.close();

  //============== getting average loads ============/
  ifstream load("/proc/loadavg");

  //checking if its open
  if(!load.is_open())
    cerr << "failed to get load average";
  
  string time, time2, time3;
  load >> time >> time2 >> time3;
  cout << "\nload average over the last minute: ";
  cout << time << ' ' << time2 << ' ' << time3 << endl << endl;

  //closing the file
  load.close();

}

int main(int argc, char * argv[])
{
  if(argc != 3)
  {
    cerr << "a.out 'period' 'interval'\n";
    return 0;
  }

  int period = atoi( argv[1]);
  int interval = atoi(argv[2]);

  for(int i = 1; i != interval/period; ++i)
  {
	  report();
	  sleep(period);
  }
  return 0;
}


