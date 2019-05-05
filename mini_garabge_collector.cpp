/*
*       mini garbage collector 
*       Author : Taha Nebti
*       github : github.com/tahanebti
*/


#include <bits/stdc++.h>

using namespace std;
// Base class for all objects that are tracked by
// the garbage collector.

namespace _tn
{
class Gc_object {
 public:

  // For mark and sweep algorithm. When a GC occurs
  // all live objects are traversed and mMarked is
  // set to true. This is followed by the sweep phase
  // where all unmarked objects are deleted.
  bool mMarked;

 public:
  Gc_object();
  Gc_object(Gc_object const&);
  virtual ~Gc_object();

  // Mark the object and all its children as live
  void mark();

  // Overridden by derived classes to call mark()
  // on objects referenced by this object. The default
  // implemention does nothing.
  virtual void markChildren();
};

// Wrapper for an array of bytes managed by the garbage
// collector.
class Gc_memory : public Gc_object {
 public:
  unsigned char* mMemory;
  int   mSize;

 public:
  Gc_memory(int size);
  virtual ~Gc_memory();

  unsigned char* get();
  int size();
};

// Garbage Collector. Implements mark and sweep GC algorithm.
class garbage_collector {
 public:
  // A collection of all active heap objects.
  typedef std::set<Gc_object*> obj_set;
  obj_set mHeap;

  // Collection of objects that are scanned for garbage.
  obj_set mRoots;

  // Pinned objects
  typedef std::map<Gc_object*, unsigned int> pinned_set;
  pinned_set mPinned;

  // Global garbage collector object
  static garbage_collector GC;

 public:
  // Perform garbage collection. If 'verbose' is true then
  // GC stats will be printed to stdout.
  void collect(bool verbose = false);

  // Add a root object to the collector.
  void addRoot(Gc_object* root);

  // Remove a root object from the collector.
  void removeRoot(Gc_object* root);

  // Pin an object so it temporarily won't be collected.
  // Pinned objects are reference counted. Pinning it
  // increments the count. Unpinning it decrements it. When
  // the count is zero then the object can be collected.
  void pin(Gc_object* o);
  void unpin(Gc_object* o);

  // Add an heap allocated object to the collector.
  void addObject(Gc_object* o);

  // Remove a heap allocated object from the collector.
  void removeObject(Gc_object* o);

  // Go through all objects in the heap, unmarking the live
  // objects and destroying the unreferenced ones.
  void sweep(bool verbose);

  // Number of live objects in heap
  int live();
};



// Gc_object
Gc_object::Gc_object() :
  mMarked(false) {
  garbage_collector::GC.addObject(this);
}

Gc_object::Gc_object(Gc_object const&) :
  mMarked(false) {
  garbage_collector::GC.addObject(this);
}

Gc_object::~Gc_object() {
}

void Gc_object::mark() {
  if (!mMarked) {
    mMarked = true;
    markChildren();
  }
}

void Gc_object::markChildren() {
}

// Gc_memory
Gc_memory::Gc_memory(int size) : mSize(size) {
  mMemory = new unsigned char[size];
}

Gc_memory::~Gc_memory() {
  delete [] mMemory;
}

unsigned char* Gc_memory::get() {
  return mMemory;
}

int Gc_memory::size() {
  return mSize;
}

// garbage_collector
garbage_collector garbage_collector::GC;

void garbage_collector::collect(bool verbose) {
  using namespace std::chrono;
  static auto const epoch = steady_clock::now();
  unsigned int start = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::steady_clock::now() - epoch ).count() / 1000000;

  // Mark root objects
  for (obj_set::iterator it = mRoots.begin();
       it != mRoots.end();
       ++it)
    (*it)->mark();

  // Mark pinned objects
  for (pinned_set::iterator it = mPinned.begin();
       it != mPinned.end();
       ++it)
    (*it).first->mark();

  if (verbose) {
    cout << "Roots: " << mRoots.size() << endl;
    cout << "Pinned: " << mPinned.size() << endl;
    cout << "GC: " << mHeap.size() << " objects in heap" << endl;
  }

  sweep(verbose);

  if (verbose) {
    unsigned int end = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::steady_clock::now() - epoch ).count() / 1000000;
    cout << "GC: " << (end-start) << " microseconds" << endl;
  }
}

void garbage_collector::addRoot(Gc_object* root) {
  mRoots.insert(root);
}

void garbage_collector::removeRoot(Gc_object* root) {
  mRoots.erase(root);
}

void garbage_collector::pin(Gc_object* o) {
  pinned_set::iterator it = mPinned.find(o);
  if (it == mPinned.end()) {
    mPinned.insert(make_pair(o, 1));
  }
  else {
    (*it).second++;
  }
}

void garbage_collector::unpin(Gc_object* o) {
  pinned_set::iterator it = mPinned.find(o);
  assert(it != mPinned.end());

  if (--((*it).second) == 0)
    mPinned.erase(it);
}

void garbage_collector::addObject(Gc_object* o) {
  mHeap.insert(o);
}

void garbage_collector::removeObject(Gc_object* o) {
  mHeap.erase(o);
}

void garbage_collector::sweep(bool verbose) {
  unsigned int live = 0;
  unsigned int dead = 0;
  unsigned int total = 0;
  vector<obj_set::iterator> erase;
  for (obj_set::iterator it = mHeap.begin();
       it != mHeap.end();
       ++it) {
    Gc_object* p = *it;
    total++;
    if (p->mMarked) {
      p->mMarked = false;
      ++live;
    }
    else {
      erase.push_back(it);
    }
  }
  dead = erase.size();
  for (vector<obj_set::iterator>::iterator it = erase.begin();
       it != erase.end();
       ++it) {
    Gc_object* p = **it;
    mHeap.erase(*it);
    delete p;
  }
  if (verbose) {
    cout << "GC: " << live << " objects live after sweep" << endl;
    cout << "GC: " << dead << " objects dead after sweep" << endl;
  }
}

int garbage_collector::live() {
  return mHeap.size();
}


class Test : public Gc_object {
};

// If the object maintains references to other GC managed objects it
// should override 'markChildren' to call 'mark' on those objects:

class Test2 : public Gc_object {
private:
  Test* mTest;

public:
  virtual void markChildren() {
    mTest->mark();
  }
};

// Periodic calls to garbage_collector::GC.collect() should be made to
// delete unreferenced objects and free memory. This call will call
// 'mark' on all the root objects, ensuring that they and their children
// are not deleted, and and remaining objects are destroyed.

// To add an object as a root, call garbage_collector::GC.addRoot().

}

int main() {
  {
    std::vector<_tn::Test *> k( 1000 );
    for( int i = 0; i < 1000; ++i ) {
      k[i] = new _tn::Test();
      k[i]->mark(); // mark obj as live
    }
    _tn::garbage_collector::GC.collect(true);
  }

  _tn::garbage_collector::GC.collect(true);
}

