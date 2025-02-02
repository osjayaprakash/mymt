// $Id: lmtable.h 3686 2010-10-15 11:55:32Z bertoldi $

/******************************************************************************
IrstLM: IRST Language Model Toolkit
Copyright (C) 2006 Marcello Federico, ITC-irst Trento, Italy

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

******************************************************************************/


#ifndef MF_LMTABLE_H
#define MF_LMTABLE_H

#ifndef WIN32
#include <sys/types.h>
#include <sys/mman.h>
#endif

#include <math.h>
#include <cstdlib>
#include <string>
#include <set>
#include <limits>
#include "util.h"
#include "ngramcache.h"
#include "dictionary.h"
#include "n_gram.h"
#include "lmContainer.h"

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define LMTMAXLEV  20
#define MAX_LINE  100000

#ifndef  LMTCODESIZE
#define  LMTCODESIZE  (int)3
#endif

#define SHORTSIZE (int)2
#define PTRSIZE   (int)sizeof(char *)
#define INTSIZE   (int)4
#define CHARSIZE  (int)1

#define PROBSIZE  (int)4 //use float  
#define QPROBSIZE (int)1 //use qfloat_t 
//#define BOUNDSIZE (int)4 //use table_pos_t
#define BOUNDSIZE (int)sizeof(table_entry_pos_t) //use table_pos_t

#define UNIGRAM_RESOLUTION 10000000.0

typedef enum {INTERNAL,QINTERNAL,LEAF,QLEAF} LMT_TYPE;
//typedef enum {BINARY,TEXT,YRANIB,NONE} OUTFILE_TYPE;
typedef char* node;

typedef enum {LMT_FIND,    //!< search: find an entry
              LMT_ENTER,   //!< search: enter an entry
              LMT_INIT,    //!< scan: start scan
              LMT_CONT     //!< scan: continue scan
             } LMT_ACTION;

typedef unsigned int  table_entry_pos_t; //type for pointing to a full ngram in the table
typedef unsigned long table_pos_t; // type for pointing to a single char in the table
typedef unsigned char qfloat_t; //type for quantized probabilities

//CHECK this part to HERE

#define BOUND_EMPTY1 (numeric_limits<table_entry_pos_t>::max() - 2)
#define BOUND_EMPTY2 (numeric_limits<table_entry_pos_t>::max() - 1)

class lmtable: public lmContainer
{
  static const bool debug=true;

  void loadtxt(std::istream& inp,const char* header,const char* filename,int mmap);
  void loadtxt_ram(std::istream& inp,const char* header);
  void loadtxt_mmap(std::istream& inp,const char* header,const char* outfilename);
  void loadtxt_level(std::istream& inp,int l);

  void loadbin(std::istream& inp,const char* header,const char* filename,int mmap);
  void loadbin_header(std::istream& inp, const char* header);
  void loadbin_dict(std::istream& inp);
  void loadbin_codebook(std::istream& inp,int l);
  void loadbin_level(std::istream& inp,int l);

protected:
  char*       table[LMTMAXLEV+1];  //storage of all levels
  LMT_TYPE    tbltype[LMTMAXLEV+1];  //table type for each levels
  table_entry_pos_t       cursize[LMTMAXLEV+1];  //current size of levels
  table_entry_pos_t       maxsize[LMTMAXLEV+1];  //max size of levels
  table_entry_pos_t*     startpos[LMTMAXLEV+1];  //support vector to store start positions
  char      info[100]; //information put in the header

  //statistics
  int    totget[LMTMAXLEV+1];
  int    totbsearch[LMTMAXLEV+1];

  //probability quantization
  bool      isQtable;

  //Incomplete LM table from distributed training
  bool      isItable;

  //Table with reverted n-grams for fast access
  bool      isInverted;

  //Table might contain pruned n-grams
  bool      isPruned;

  int       NumCenters[LMTMAXLEV+1];
  float*    Pcenters[LMTMAXLEV+1];
  float*    Bcenters[LMTMAXLEV+1];

  double  logOOVpenalty; //penalty for OOV words (default 0)
  int     dictionary_upperbound; //set by user
  int     backoff_state;

  //improve access speed
  int max_cache_lev;

  NGRAMCACHE_t* prob_and_state_cache;
  NGRAMCACHE_t* lmtcache[LMTMAXLEV+1];
  float ngramcache_load_factor;
  float dictionary_load_factor;

  //memory map on disk
  int memmap;  //level from which n-grams are accessed via mmap
  int diskid;
  off_t tableOffs[LMTMAXLEV+1];
  off_t tableGaps[LMTMAXLEV+1];

  // is this LM queried for knowing the matching order or (standard
  // case) for score?
  bool      orderQuery;

public:

#ifdef TRACE_CACHELM
  std::fstream* cacheout;
  int sentence_id;
#endif

  dictionary     *dict; // dictionary (words - macro tags)

  lmtable(float nlf=0.0, float dlfi=0.0);

  virtual ~lmtable();

  table_entry_pos_t wdprune(float *thr, int aflag=0);
  table_entry_pos_t wdprune(float *thr, int aflag, ngram ng, int ilev, int elev, table_entry_pos_t ipos, table_entry_pos_t epos, double lk=0, double bo=0, double *ts=0, double *tbs=0);
  double lprobx(ngram ong, double *lkp=0, double *bop=0, int *bol=0);

  table_entry_pos_t ngcnt(table_entry_pos_t *cnt);
  table_entry_pos_t ngcnt(table_entry_pos_t *cnt, ngram ng, int l, table_entry_pos_t ipos, table_entry_pos_t epos);
  int pscale(int lev, table_entry_pos_t ipos, table_entry_pos_t epos, double s);

  void init_prob_and_state_cache();
  void init_probcache() {
    init_prob_and_state_cache();
  }; //kept for back compatibility
  void init_statecache() {}; //kept for back compatibility
  void init_lmtcaches(int uptolev);
  void init_caches(int uptolev);

  void used_prob_and_state_cache();
  void used_lmtcaches();
  void used_caches();


  void delete_prob_and_state_cache();
  void delete_probcache() {
    delete_prob_and_state_cache();
  }; //kept for back compatibility
  void delete_statecache() {}; //kept for back compatibility
  void delete_lmtcaches();
  void delete_caches();

  void check_prob_and_state_cache_levels();
  void check_probcache_levels() {
    check_prob_and_state_cache_levels();
  }; //kept for back compatibility
  void check_statecache_levels() {}; //kept for back compatibility
  void check_lmtcaches_levels();
  void check_caches_levels();

  void reset_prob_and_state_cache();
  void reset_probcache() {
    reset_prob_and_state_cache();
  }; //kept for back compatibility
  void reset_statecache() {}; //kept for back compatibility
  void reset_lmtcaches();
  void reset_caches();


  bool are_prob_and_state_cache_active();
  bool is_probcache_active() {
    return are_prob_and_state_cache_active();
  }; //kept for back compatibility
  bool is_statecache_active() {
    return are_prob_and_state_cache_active();
  }; //kept for back compatibility
  bool are_lmtcaches_active();
  bool are_caches_active();

  void reset_mmap();

//set the inverted flag to load ngrams in an inverted order
//this choice is disregarded if a binary LM is loaded,
//because the info is stored into the header
  bool is_inverted(const bool flag) {
    return isInverted=flag;
  }
  bool is_inverted() {
    return isInverted;
  }

  void configure(int n,bool quantized);

  //set penalty for OOV words
  double getlogOOVpenalty() const {
    return logOOVpenalty;
  }

  double setlogOOVpenalty(int dub) {
    assert(dub > dict->size());
    dictionary_upperbound = dub;
    return logOOVpenalty=log((double)(dictionary_upperbound - dict->size()))/M_LN10;
  }

  double setlogOOVpenalty(double oovp) {
    return logOOVpenalty=oovp;
  }

  virtual int maxlevel() const {
    return maxlev;
  };
  bool isQuantized() const {
    return isQtable;
  }


  void savetxt(const char *filename);
  void savebin(const char *filename);

  void savebin_level(int level, const char* filename, int mmap);
  void savebin_level_nommap(int level, const char* filename);
  void savebin_level_mmap(int level, const char* filename);
  void savebin_dict(std::fstream& out);

  void compact_level(int level, const char* outfilename);


  void print_table_stat();
  void print_table_stat(int level);

  void dumplm(std::fstream& out,ngram ng, int ilev, int elev, table_entry_pos_t ipos,table_entry_pos_t epos);


  void resize_level(int level, const char* outfilename, int mmap);
  void resize_level_nommap(int level);
  void resize_level_mmap(int level, const char* filename);

  void load(const std::string filename, int mmap=0);
  void load(std::istream& inp,const char* filename=NULL,const char* outfilename=NULL,int mmap=0,OUTFILE_TYPE outtype=NONE);

  void load_centers(std::istream& inp,int l);


  void expand_level(int level, table_entry_pos_t size, const char* outfilename, int mmap);
  void expand_level_nommap(int level, table_entry_pos_t size);
  void expand_level_mmap(int level, table_entry_pos_t size, const char* outfilename);
  void cpsublm(lmtable* sublmt, dictionary* subdict,bool keepunigr=true);

  int reload(std::set<string> words);

  void filter(const char* /* unused parameter: lmfile */) {};


  virtual double  lprob(ngram ng, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL, bool* extendible=NULL, double* lastbow=NULL);
  virtual double clprob(ngram ng, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL);
  virtual double clprob(int* ng, int ngsize, double* bow=NULL,int* bol=NULL,char** maxsuffptr=NULL,unsigned int* statesize=NULL,bool* extendible=NULL);


  void *search(int lev,table_entry_pos_t offs,table_entry_pos_t n,int sz,int *w, LMT_ACTION action,char **found=(char **)NULL);

  int mybsearch(char *ar, table_entry_pos_t n, int size, char *key, table_entry_pos_t *idx);


  template<typename TA, typename TB> int add(ngram& ng, TA prob,TB bow);

  void checkbounds(int level);

  inline int get(ngram& ng) {
    return get(ng,ng.size,ng.size);
  }
  int get(ngram& ng,int n,int lev);

  int succscan(ngram& h,ngram& ng,LMT_ACTION action,int lev);

  virtual const char *maxsuffptr(ngram ong, unsigned int* size=NULL);
  virtual const char *cmaxsuffptr(ngram ong, unsigned int* size=NULL);

  inline void putmem(char* ptr,int value,int offs,int size) {
    assert(ptr!=NULL);
    for (int i=0; i<size; i++)
      ptr[offs+i]=(value >> (8 * i)) & 0xff;
  };

  inline void getmem(char* ptr,int* value,int offs,int size) {
    assert(ptr!=NULL);
    *value=ptr[offs] & 0xff;
    for (int i=1; i<size; i++)
      *value= *value | ( ( ptr[offs+i] & 0xff ) << (8 *i));
  };
  template<typename T>
  inline void putmem(char* ptr,T value,int offs) {
    assert(ptr!=NULL);
    memcpy(ptr+offs, &value, sizeof(T));
  };

  template<typename T>
  inline void getmem(char* ptr,T* value,int offs) {
    assert(ptr!=NULL);
    memcpy((void*)value, ptr+offs, sizeof(T));
  };


  int nodesize(LMT_TYPE ndt) {
    switch (ndt) {
    case INTERNAL:
      return LMTCODESIZE + PROBSIZE + PROBSIZE + BOUNDSIZE;
    case QINTERNAL:
      return LMTCODESIZE + QPROBSIZE + QPROBSIZE + BOUNDSIZE;
    case LEAF:
      return LMTCODESIZE + PROBSIZE;
    case QLEAF:
      return LMTCODESIZE + QPROBSIZE;
    default:
      assert(0);
      return 0;
    }
  }

  inline int word(node nd,int value=-1) {
    int offset=0;

    if (value==-1)
      getmem(nd,&value,offset,LMTCODESIZE);
    else
      putmem(nd,value,offset,LMTCODESIZE);

    return value;
  };


  int codecmp(node a,node b) {
    register int i,result;
    for (i=(LMTCODESIZE-1); i>=0; i--) {
      result=(unsigned char)a[i]-(unsigned char)b[i];
      if(result) return result;
    }
    return 0;
  };

  int codediff(node a,node b) {
    return word(a)-word(b);
  };


  inline float prob(node nd,LMT_TYPE ndt) {
    int offs=LMTCODESIZE;

    float fv;
    unsigned char cv;
    switch (ndt) {
    case INTERNAL:
      getmem(nd,&fv,offs);
      return fv;
    case QINTERNAL:
      getmem(nd,&cv,offs);
      return (float) cv;
    case LEAF:
      getmem(nd,&fv,offs);
      return fv;
    case QLEAF:
      getmem(nd,&cv,offs);
      return (float) cv;
    default:
      assert(0);
      return 0;
    }
  };

  template<typename T>
  inline float prob(node nd,LMT_TYPE /* unused parameter: ndt */, T value) {
    int offs=LMTCODESIZE;

    putmem(nd,value,offs);

    return (float) value;
  };


  inline float bow(node nd,LMT_TYPE ndt) {
    int offs=LMTCODESIZE+(ndt==QINTERNAL?QPROBSIZE:PROBSIZE);

    float fv;
    unsigned char cv;
    switch (ndt) {
    case INTERNAL:
      getmem(nd,&fv,offs);
      return fv;
    case QINTERNAL:
      getmem(nd,&cv,offs);
      return (float) cv;
    case LEAF:
      getmem(nd,&fv,offs);
      return fv;
    case QLEAF:
      getmem(nd,&cv,offs);
      return (float) cv;
    default:
      assert(0);
      return 0;
    }
  };

  template<typename T>
  inline T bow(node nd,LMT_TYPE ndt, T value) {
    int offs=LMTCODESIZE+(ndt==QINTERNAL?QPROBSIZE:PROBSIZE);

    putmem(nd,value,offs);

    return value;
  };

  inline table_entry_pos_t bound(node nd,LMT_TYPE ndt) {

    int offs=LMTCODESIZE+2*(ndt==QINTERNAL?QPROBSIZE:PROBSIZE);

    table_entry_pos_t v;

    getmem(nd,&v,offs);
    return v;
  };



  template<typename T>
  inline T bound(node nd,LMT_TYPE ndt, T value) {

    int offs=LMTCODESIZE+2*(ndt==QINTERNAL?QPROBSIZE:PROBSIZE);

    putmem(nd,value,offs);

    return value;
  };

  //returns the indexes of the successors of a node
  int succrange(node ndp,int level,table_entry_pos_t* isucc=NULL,table_entry_pos_t* esucc=NULL);

  void stat(int lev=0);
  void printTable(int level);

  virtual inline void setDict(dictionary* d) {
    dict=d;
  };
  virtual inline dictionary* getDict() const {
    return dict;
  };

  inline table_entry_pos_t getCurrentSize(int l) const {
    return cursize[l];
  };

  inline void setOrderQuery(bool v) {
    orderQuery = v;
  }
  inline bool isOrderQuery() const {
    return orderQuery;
  }

  inline float GetNgramcacheLoadFactor() {
    return  ngramcache_load_factor;
  }
  inline float GetDictioanryLoadFactor() {
    return  ngramcache_load_factor;
  }

//never allow the increment of the dictionary through this function
  inline virtual void dictionary_incflag(const bool flag) {
    UNUSED(flag);
  };

  inline virtual bool filter(const string sfilter, lmtable* sublmt, const string skeepunigrams) {
    std::cerr << "filtering... \n";
    dictionary *dict=new dictionary((char *)sfilter.c_str());

    cpsublm(sublmt, dict,(skeepunigrams=="yes"));
    delete dict;
    std::cerr << "...done\n";
    return true;
  }

  inline virtual bool is_OOV(int code) {
    return (code == dict->oovcode());
  };
};

#endif

