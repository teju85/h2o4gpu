#ifdef __JETBRAINS_IDE__
    #define __host__
    #define __device__
#endif

#include <sstream>
#include <stdio.h>

// Check CUDA calls
#define CUDACHECK(cmd) do {                         \
    cudaError_t e = cmd;                              \
    if( e != cudaSuccess ) {                          \
      printf("Cuda failure %s:%d '%s'\n",             \
             __FILE__,__LINE__,cudaGetErrorString(e));   \
      exit(EXIT_FAILURE);                             \
    }                                                 \
  } while(0)


#ifdef USE_NCCL2
#include "nccl.h"

#include <curand.h>
#include <cerrno>
#include <string>

// Propagate errors up
#define NCCLCHECK(cmd) do {                         \
    ncclResult_t r = cmd;                             \
    if (r!= ncclSuccess) {                            \
      printf("NCCL failure %s:%d '%s'\n",             \
             __FILE__,__LINE__,ncclGetErrorString(r));   \
      exit(EXIT_FAILURE);                             \
    }                                                 \
  } while(0)


#endif // end if USE_NCCL defined



#ifdef USE_NVTX
#include <nvToolsExt.h>

const uint32_t colors[] = { 0x0000ff00, 0x000000ff, 0x00ffff00, 0x00ff00ff, 0x0000ffff, 0x00ff0000, 0x00ffffff , 0x00f0ffff , 0x000fffff  , 0x00f0f0ff , 0x000ff0f0 };
const int num_colors = sizeof(colors)/sizeof(uint32_t);

// whether push/pop timer is enabled (1) or not (0)
//#define PUSHPOPTIMER 1

#define PUSH_RANGE(name,tid,cid) \
  { \
    fprintf(stderr,"START: name=%s cid=%d\n",name,cid); fflush(stderr); \
    int color_id = cid; \
    color_id = color_id%num_colors;\
    nvtxEventAttributes_t eventAttrib = {0}; \
    eventAttrib.version = NVTX_VERSION; \
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE; \
    eventAttrib.colorType = NVTX_COLOR_ARGB; \
    eventAttrib.color = colors[color_id]; \
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII; \
    eventAttrib.message.ascii = name; \
    nvtxRangePushEx(&eventAttrib); \
  }\
    double timer##tid = timer<double>();

#define POP_RANGE(name,tid,cid) {                                       \
    fprintf(stderr,"STOP:  name=%s cid=%d duration=%g\n",name,cid,timer<double>() - timer##tid); fflush(stderr); \
    nvtxRangePop(); \
  }
#else
#define PUSH_RANGE(name,tid,cid)
#define POP_RANGE(name,tid,cid)
#endif


#ifndef H2OAIGLM_H_
#define H2OAIGLM_H_

#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <iterator>

#include "projector/projector_direct.h"
#include "projector/projector_cgls.h"
#include "prox_lib.h"


namespace h2oaiglm {

static const std::string H2OAIGLM_VERSION = "0.3.1";

// TODO: Choose default constants better
// Defaults.
const double       kAbsTol      = 1e-4;
const double       kRelTol      = 1e-3;
const double       kRhoInit     = 1.;
const unsigned int kVerbose     = 2u;   // 0...4
const unsigned int kMaxIter     = 2500u;
const unsigned int kInitIter    = 10u;
const bool         kAdaptiveRho = true;
const bool         kEquil       = true;
const bool         kGapStop     = false;
const int          knDev        = 1;
const int          kwDev        = 0;

#define __HBAR__ \
"----------------------------------------------------------------------------\n"


// Status messages
enum H2OAIGLMStatus { H2OAIGLM_SUCCESS,    // Converged successfully.
                  H2OAIGLM_INFEASIBLE, // Problem likely infeasible.
                  H2OAIGLM_UNBOUNDED,  // Problem likely unbounded
                  H2OAIGLM_MAX_ITER,   // Reached max iter.
                  H2OAIGLM_NAN_FOUND,  // Encountered nan.
                  H2OAIGLM_ERROR };    // Generic error, check logs.

// Proximal Operator Graph Solver.
template <typename T, typename M, typename P>
class H2OAIGLM {
 private:
  // Data
  M _A;
  P _P;
  T *_z, *_zt, _rho;
  bool _done_init;

  // Output for user (always on CPU)
  T *_x, *_y, *_mu, *_lambda, _optval, _time;
  T *_trainPreds, *_validPreds;
  // Output for internal post-processing (e.g. on GPU if GPU or on CPU if CPU)
  T *_xp, *_trainPredsp, *_validPredsp;

  T _trainrmse, _validrmse;
  T _trainmean, _validmean;
  T _trainstddev, _validstddev;
  unsigned int _final_iter;

  // Parameters.
  T _abs_tol, _rel_tol;
  unsigned int _max_iter, _init_iter, _verbose;
  bool _adaptive_rho, _equil, _gap_stop, _init_x, _init_lambda;
    // cuda number of devices and which device(s) to use
  int _nDev,_wDev;
  // NCCL communicator
#ifdef USE_NCCL2
  ncclComm_t* _comms;
#endif

    // Setup matrix _A and solver _LS
  int _Init();


 public:
  // Constructor and Destructor.
  H2OAIGLM(int sharedA, int me, int wDev, const M &A);
  H2OAIGLM(const M &A);
  ~H2OAIGLM();

  // Solve for specific objective.
  H2OAIGLMStatus Solve(const std::vector<FunctionObj<T> >& f,
                   const std::vector<FunctionObj<T> >& g);
  void ResetX(void);

  // Getters for solution variables and parameters.
  const T*     GetX()           const { return _x; }
  const T*     GetY()           const { return _y; }
  const T*     GetLambda()      const { return _lambda; }
  const T*     GetMu()          const { return _mu; }
  T            GetOptval()      const { return _optval; }
  const T*     GettrainPreds()  const { return _trainPreds; }
  const T*     GetvalidPreds()  const { return _validPreds; }
  unsigned int GetFinalIter()   const { return _final_iter; }
  T            GetRho()         const { return _rho; }
  T            GetRelTol()      const { return _rel_tol; }
  T            GetAbsTol()      const { return _abs_tol; }
  unsigned int GetMaxIter()     const { return _max_iter; }
  unsigned int GetInitIter()    const { return _init_iter; }
  unsigned int GetVerbose()     const { return _verbose; }
  bool         GetAdaptiveRho() const { return _adaptive_rho; }
  bool         GetEquil()       const { return _equil; }
  bool         GetGapStop()     const { return _gap_stop; }
  T            GetTime()        const { return _time; }
  int          GetnDev()        const { return _nDev; }
  int          GetwDev()        const { return _wDev; }

  void printMe(std::ostream &os, T fa, T fb, T fc, T fd, T fe, T ga, T gb, T gc, T gd, T ge) const {
    os << "Model parameters: ";
    std::string sep = ", ";
    os << "version: " << _GITHASH_ << sep;
    os << "f.abcde: " << fa << sep<< fb << sep<< fc << sep<< fd << sep<< fe << sep;
    os << "g.abcde: " << ga << sep<< gb << sep<< gc << sep<< gd << sep<< ge << sep;
    os << "rho: " << _rho << sep;
    os << "rel_tol: " << _rel_tol << sep;
    os << "abs_tol: " << _abs_tol << sep;
    os << "max_iter: " << _max_iter << sep;
    os << "init_iter: " << _init_iter << sep;
    os << "verbose: " << _verbose << sep;
    os << "adaptive_rho: " << _adaptive_rho << sep;
    os << "equil: " << _equil << sep;
    os << "gap_stop: " << _gap_stop << sep;
    os << "nDev: " << _nDev << sep;
    os << "wDev: " << _wDev;
    os << std::endl;
  }
  void printData(std::ostream &os) const {
    os << "Model training data: ";
    std::copy(_A.Data(), _A.Data() + _A.Rows()*_A.Cols(), std::ostream_iterator<T>(std::cout, "\n"));
    os << std::endl;
  }
  void printLegalNotice() {
#pragma omp critical
    Printf(__HBAR__
        "           H2O AI GLM\n"
        "           Version: %s %s\n"
        "           Compiled: %s %s\n"
        "           (c) H2O.ai, Inc., 2017\n"
        "           based on\n"
        "           H2OAIGLM 0.2.0 - Proximal Graph Solver\n"
        "           (c) Christopher Fougner, Stanford University 2014-2015\n"
        __HBAR__,
        H2OAIGLM_VERSION.c_str(),
        _GITHASH_,
        __DATE__,
        __TIME__);
  }

  // Setters for parameters and initial values.
  void SetRho(T rho)                       { _rho = rho; }
  void SetAbsTol(T abs_tol)                { _abs_tol = abs_tol; }
  void SetRelTol(T rel_tol)                { _rel_tol = rel_tol; }
  void SetMaxIter(unsigned int max_iter)   { _max_iter = max_iter; }
  void SetInitIter(unsigned int init_iter) { _init_iter = init_iter; }
  void SetVerbose(unsigned int verbose)    { _verbose = verbose; }
  void SetAdaptiveRho(bool adaptive_rho)   { _adaptive_rho = adaptive_rho; }
  void SetEquil(bool equil)                { _equil = equil; }
  void SetGapStop(bool gap_stop)           { _gap_stop = gap_stop; }
  void SetnDev(int nDev)    { _nDev = nDev; }
  void SetwDev(int wDev)    { _wDev = wDev; }
  void SetInitX(const T *x) {
    memcpy(_x, x, _A.Cols() * sizeof(T));
    _init_x = true;
  }
  void SetInitLambda(const T *lambda) {
    memcpy(_lambda, lambda, _A.Rows() * sizeof(T));
    _init_lambda = true;
  }
};

// Templated typedefs
#ifndef __CUDACC__
template <typename T, typename M>
using H2OAIGLMDirect = H2OAIGLM<T, M, ProjectorDirect<T, M> >;

template <typename T, typename M>
using H2OAIGLMIndirect = H2OAIGLM<T, M, ProjectorCgls<T, M> >;
#endif

// String version of status message.
inline std::string H2OAIGLMStatusString(H2OAIGLMStatus status) {
  switch(status) {
    case H2OAIGLM_SUCCESS:
      return "Solved";
    case H2OAIGLM_UNBOUNDED:
      return "Unbounded";
    case H2OAIGLM_INFEASIBLE:
      return "Infeasible";
    case H2OAIGLM_MAX_ITER:
      return "Reached max iter";
    case H2OAIGLM_NAN_FOUND:
      return "Encountered NaN";
    case H2OAIGLM_ERROR:
    default:
      return "Error";
  }
}

}  // namespace h2oaiglm

#endif  // H2OAIGLM_H_

