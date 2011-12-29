//
// threads.H
//

#include <pthread.h>
#include <semaphore.h>
#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <queue>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

using namespace std;

inline string itoa(long int x) { ostringstream s;s<<x;return s.str(); }
#define EXCLUSION Sentry exclusion(this); exclusion.touch();

extern string Me();
extern string report( );  

// ====================== priority aueue ============================


// pQueue behaves exactly like STL's queue, except that push has an
// optional integer priority argument that defaults to INT_MAX.
// front() returns a reference to the item with highest priority
// (i.e., smallest priority number) with ties broken on a FIFO basis.
// It may overflow and malfunction after INT_MAX pushes.

template<class T>                // needed for priority comparisions
bool operator<( const pair<pair<int,int>,T>& a, 
                const pair<pair<int,int>,T>& b  
              ) {
  return a.first.first > b.first.first ? true  :
         a.first.first < b.first.first ? false :
               a.first.second > b.first.second ;
} 

template <class T>
class pQueue {
  priority_queue< pair<pair<int,int>,T> > q;  
  int i;  // counts invocations of push to break ties on FIFO basis.
public:
  pQueue() : i(0) {;}
  void push( T t, int n = INT_MAX ) { 
    q.push( pair<pair<int,int>,T>(pair<int,int>(n,++i),t) ); 
  }
  void pop() { q.pop(); }
  T front() { return q.top().second; }
  int size() { return q.size(); }
  bool empty() { return q.empty(); }
};

// =========================== interrupts ======================

class InterruptSystem {
public:       // man sigsetops for details on signal operations.
  static void handler(int sig);
  // exported pseudo constants
  static sigset_t on;                    
  static sigset_t alrmoff;    
  static sigset_t alloff;     
  InterruptSystem() {  
    signal( SIGALRM, InterruptSystem::handler ); 
    sigemptyset( &on );
    sigemptyset( &alrmoff );
    sigaddset( &alrmoff, SIGALRM );
    sigfillset( &alloff );
    set( alloff );
    struct itimerval time;
    time.it_interval.tv_sec  = 0;
    time.it_interval.tv_usec = 400000;
    time.it_value.tv_sec     = 0;
    time.it_value.tv_usec    = 400000;
    cerr << "starting timer\n";
    setitimer(ITIMER_REAL, &time, NULL);
  }
  sigset_t set( sigset_t mask ) {
    sigset_t oldstatus;
    pthread_sigmask( SIG_SETMASK, &mask, &oldstatus );
    return oldstatus;
  }                // Sets interrupts and returns former status.
  sigset_t block( sigset_t mask ) {
    sigset_t oldstatus;
    pthread_sigmask( SIG_BLOCK, &mask, &oldstatus );
    return oldstatus;
  }                     // Leaves blocked those already blocked.
};

// signal mask pseudo constants
sigset_t InterruptSystem::on;   
sigset_t InterruptSystem::alrmoff; 
sigset_t InterruptSystem::alloff;  

InterruptSystem interrupts;               // singleton instance.


// ========= Threads, Semaphores, Sentry's, Monitors ===========

class Semaphore {           // C++ wrapper for Posix semaphores.
  sem_t s;                               // the Posix semaphore.
public:
  Semaphore( int n = 0 ) { assert( ! sem_init(&s, 0, n) ); }
  ~Semaphore()           { assert( ! sem_destroy(&s) );    }
  void release()         { assert( ! sem_post(&s) );       }
  void acquire() {
    sigset_t old = interrupts.set( InterruptSystem::alloff );
    sem_wait( &s );
    interrupts.set( old );
  }

//   void acquire() {
//     // The following is a diagnostic/debugging version.
//     sigset_t old = interrupts.set(InterruptSystem::alloff);
//     int temp;
//     int errsv;
//     while ( (temp=sem_wait(&s)) && ( (errsv=errno) == EINTR ) ) {
//       cerr << "errno = " << errsv << " by " << me() << endl; 
//     }
//     if ( temp && errsv != EINTR ) {
//       assert( errsv != EINVAL );   // interrupted semaphore wait.
//       assert( false );                // can't happen, but might.
//     }
//     interrupts.set(old);
//   }  

};


class Lock : public Semaphore {
public:             // A Semaphore initialized to one is a Lock.
  Lock() : Semaphore(1) {} 
};


class Monitor : Lock {
  friend class Sentry;
  friend class Condition;
  sigset_t mask;
public:
  void lock()   { Lock::acquire(); }
  void unlock() { Lock::release(); }
  Monitor( sigset_t mask = InterruptSystem::alrmoff ) 
    : mask(mask) 
  {}
};


class Sentry {            // An autoreleaser for monitor's lock.
  Monitor&  mon;           // Reference to local monitor, *this.
  const sigset_t old;    // Place to store old interrupt status.
public:
  void touch() {}          // To avoid unused-variable warnings.
  Sentry( Monitor* m ) 
    : mon( *m ), 
      old( interrupts.block(mon.mask) )
  { 
    mon.lock(); 
  }
  ~Sentry() {
    mon.unlock();
    interrupts.set( old ); 
  }
};


template< typename T1, typename T2 >
class ThreadSafeMap : Monitor {
  map<T1,T2> m;
public:
  T2& operator [] ( T1 x ) {
    EXCLUSION
    return m[x];
  }
};


class Thread {
  friend class Condition;
  friend class CPUallocator;                      // NOTE: added.
  pthread_t pt;                                    // pthread ID.
  static void* start( Thread* );
  virtual void action() = 0;
  Semaphore go;
  static ThreadSafeMap<pthread_t,Thread*> whoami;  

  virtual int priority() { 
    return INT_MAX;         // place holder for complex policies.
  }   

  void suspend() { 
    cerr << "Suspending thread " << (unsigned long)this << endl;
    go.acquire(); 
    cerr << "Unsuspended thread " << (unsigned long)this << endl;
  }

  void resume() { 
    cerr << endl << "Resuming " << long(this)  << " by " 
         << Me() << endl;
    go.release(); 
  }

  int self() { return pthread_self(); }

  void join() { assert( pthread_join(pt, NULL) ); }

public:

  static Thread* me();

  virtual ~Thread() { pthread_cancel(pt); }  

  Thread() {
    cerr << "creating thread " << (unsigned long)this << endl;
    assert( 
      0 == pthread_create(&pt, NULL, (void* (*)(void*)) start, this) 
    );
  }

};




class Condition : pQueue< Thread* > {
  Monitor&  mon;                     // reference to local monitor
public:
  Condition( Monitor* m ) : mon( *m ) {;}
  int waiting() { return size(); }
  bool awaited() { return waiting() > 0; }
  void wait( int pr = INT_MAX );    // wait() is defined after CPU
  void signal() { 
    if ( awaited() ) {
      Thread* t = front();
      pop();
      cerr << "Signalling" << endl;
      t->resume();
    }
  }
  void broadcast() { while ( awaited() ) signal(); }
}; 


// ====================== AlarmClock ===========================

class AlarmClock : Monitor {
  unsigned long now;
  unsigned long alarm;
  Condition wakeup;
public:
  AlarmClock() 
    : now(0),
      alarm(INT_MAX),
      wakeup(this)
  {;}
  int gettime() { return now; }
  void wakeme_at(int time) {
    EXCLUSION
    if ( time < alarm ) alarm = time;
    while ( now < time ) {
      cerr << " ->wakeup wait " << endl;
      wakeup.wait(time);
      cerr << " wakeup-> ";
      if ( alarm < time ) alarm = time;
    }
    alarm = INT_MAX;
    wakeup.signal();
  }
  void wakeme_in(int time) { wakeme_at(now+time); }
  void tick() { 
    EXCLUSION
    cerr << "now=" << (unsigned long)now 
         << " alarm=" << (unsigned long)alarm
         << " sleepers:" << wakeup.waiting() << endl;
    if ( now++ >= alarm && wakeup.awaited() ) wakeup.signal();
  }
};

extern AlarmClock dispatcher;                  // singleton instance.


class Idler : Thread {                       // awakens periodically.
  // Idlers wake up periodically and then go back to sleep.
  string name;
  int period;
  int priority () { return 0; }                  // highest priority.
  void action() { 
    cerr << "Idler running\n";
    int n = 0;
    for(;;) { 
      cerr << name << " " << Me() << " ";
      dispatcher.wakeme_at( ++n * period ); 
    } 
  }
public:
  Idler( string name, int period = 5 )    // period defaults to five.
    : name(name), period(period)
  {}
};


//==================== CPU-related stuff ============================ 



class CPUallocator : Monitor {                            

  // a scheduler for the pool of CPUs.
  friend string report();
  pQueue<Thread*> ready;       // queue of Threads waiting for a CPU.
  int cpu_count;           // count of unallocated (i.e., free) CPUs.

public:

  CPUallocator( int n ) : cpu_count(n) {;}

  void defer( int pr = Thread::me()->priority() ) 
  {
    EXCLUSION
    ready.push(Thread::me(), pr);
    Thread * t = ready.front();
    ready.pop();
    if(t == Thread::me()) return;
    cpu_count++;
    assert(cpu_count > 0);
    t->resume();
    unlock();
    Thread::me()->suspend();
    lock();
    while(cpu_count == 0)
    {
      ready.push(Thread::me(), pr);
      unlock();
      Thread::me()->suspend();
      lock();
    }
    cpu_count--;
  }

  void acquire( int pr = Thread::me()->priority() ) 
  {
    EXCLUSION
    while(cpu_count == 0)
    {
      ready.push(Thread::me(), pr);
      unlock();
      Thread::me()->suspend();
      lock();
    }
    cpu_count--;
  }

  void release()  
  {
    EXCLUSION
    cpu_count++;
    assert(cpu_count > 0);
    if(ready.size() > 0)
    {
      Thread * t = ready.front();
      ready.pop();
      t->resume();
    }
  }

};


/*
class CPUallocator { // makes a semaphore appear to be a pool of CPUs.
  // a scheduler for the pool of CPUs.
  Semaphore sem;
public:
  int cpu_count; // not used here, and set to visibly invalid value.
  pQueue<Thread*> ready;
  CPUallocator( int count ) : sem(count), cpu_count(-1) {}
  void defer() { sem.release(); sem.acquire(); }
  void acquire( int pr = INT_MAX ) { sem.acquire(); }
  void release() { sem.release(); }
};
*/

extern CPUallocator CPU;     // single instance. initially set here.


void InterruptSystem::handler(int sig) {                  // static.
  static int tickcount = 0;
  cerr << "TICK " << tickcount << ": "<< report() << endl; 
  dispatcher.tick(); 
  if ( ! ( (tickcount++) % 3 ) ) {
    cerr << "DEFERRING " << Me() << endl;
    CPU.defer(); // timeslice: 3 ticks.
  }
  assert( tickcount < 35 );
} 


void* Thread::start(Thread* myself) {                     // static.
  interrupts.set(InterruptSystem::alloff);
  cerr << "Starting thread " << long(myself)
       << " pt=" << (unsigned long)pthread_self() << endl; 
  assert( myself );
  whoami[ pthread_self() ] = myself;
  assert ( Thread::me() == myself );
  interrupts.set(InterruptSystem::on);
  cerr << "waiting for my first CPU ..." << Me() << endl;
  CPU.acquire();  
  cerr << "got my first CPU, by " << Me() << endl;
  myself->action();
  cerr << "Exiting and releasing cpu, by " << Me() << endl;
  CPU.release();
  pthread_exit(NULL);
}


void Condition::wait( int pr ) {
  push( Thread::me(), pr );
  cerr << "Releasing CPU just prior to wait: " << Me() << endl;
  mon.unlock();
  CPU.release();  
  cerr << "WAITING: " << Me() << endl;
  Thread::me()->suspend();
  CPU.acquire();  
  mon.lock();
}


// ================ application stuff  =============================

class InterruptCatcher : Thread {
public:
  // A thread to catch interrupts, when no other thread will.
  int priority () { return 1; }          // second highest priority.
  void action() { 
    cerr << "\nInterrupt catcher (" << Me() << ") now running\n";
    for(;;) {
      CPU.release();         
      pause(); 
      cerr << "InterruptCatcher ("  << Me() << ") requesting CPU.\n";
      CPU.acquire();
    }
  }
};


class Pauser {                            // none created so far.
public:
  Pauser() { pause(); }
};


// diagnostic functions.

string report() {               
  // diagnostic report on number of unassigned cpus, number of 
  // threads waiting for cpus, and which thread is first in line.
  ostringstream s;
  string tag = ""; // make this a parameter.
  s << tag << CPU.cpu_count << "/" << CPU.ready.size() << "/" 
    << (CPU.ready.size() ? (unsigned long) CPU.ready.front() : 0) 
    << " by " << Me();
  return s.str(); 
}


// ================== threads.cc ===================================

// Single-instance globals

Idler idler(" IDLER ");                        // single instance.
InterruptCatcher theInterruptCatcher;          // single instance.
AlarmClock dispatcher;                         // single instance.
CPUallocator CPU(1);                 // single instance, set here.
Thread* Thread::me() { return whoami[pthread_self()]; }  // static
string Me() { return itoa((unsigned long)Thread::me()); }
               // NOTE: Thread::start() is defined after class CPU
ThreadSafeMap<pthread_t,Thread*> Thread::whoami;         // static


// =================== main.cc ======================================

//
// main.cc 
//

// applied monitors and thread classes for testing

class SharableInteger: public Monitor {
public:
  int data;
  SharableInteger() : data(0) {;}
  void increment() {
    EXCLUSION
    ++data;
  }
  string show() {
    EXCLUSION
    return itoa(data);
  }  
} counter;                                        // single instance


class Incrementer : Thread {
  int priority() { return INT_MAX - 1; }            // high priority
  void action() { 
    cerr << "\nNew incrementer running: " << Me() << endl;
    for(int i= 0; i < 120; ++i) {
      for(int i=0;i<12000000;++i) {}  // delay
      counter.increment();
      cerr << "Thread " << Me() << " Counter : " 
           << counter.show() << endl;
    }
    cerr << Me() << " done\n";
  }
};


// ======= control order of construction of initial threads  ========


// Create and run three concurrent Incrementers for the single global 
// SharableInteger counter as a test case.
Incrementer t1;
Incrementer t2;
Incrementer t3;

int main( int argc, char *argv[] ) {
  // shutting down the main thread
  interrupts.set( InterruptSystem::alloff );  // for this thread only
  pause();                           // stops until interrupt arrives
  assert( false );                            // and none ever should
}
