
// @SCALFMM_PRIVATE
#ifndef FSTARPUUTILS_HPP
#define FSTARPUUTILS_HPP

/////////////////////////////////////////////////////
#include "../../Utils/FGlobal.hpp"

/////////////////////////////////////////////////////

#include <starpu.h>

/////////////////////////////////////////////////////

#if (STARPU_MAJOR_VERSION >= 1) && (STARPU_MINOR_VERSION >= 2)
#define STARPU_SUPPORT_COMMUTE
#endif

/////////////////////////////////////////////////////

#if defined(STARPU_USE_CUDA) && defined(SCALFMM_USE_CUDA)
#define SCALFMM_ENABLE_CUDA_KERNEL
#else
    #if defined(STARPU_USE_CUDA)
        #warning CUDA is turned off because it is not supported by ScalFMM.
    #endif
    #if defined(SCALFMM_USE_CUDA)
        #warning CUDA is turned off because it is not supported by StarPU.
    #endif
#endif

/////////////////////////////////////////////////////

#if defined(STARPU_USE_OPENCL) && defined(SCALFMM_USE_OPENCL)
#define SCALFMM_ENABLE_OPENCL_KERNEL
#else
    #if defined(STARPU_USE_OPENCL)
        #warning OPENCL is turned off because it is not supported by ScalFMM.
    #endif
    #if defined(SCALFMM_USE_OPENCL)
        #warning OPENCL is turned off because it is not supported by StarPU.
    #endif
#endif

/////////////////////////////////////////////////////

#if defined(STARPU_USE_MPI) && defined(SCALFMM_USE_MPI)
    #if !defined(SCALFMM_USE_MPI)
        #warning Cannot may not link because MPI is needed by starpu.
    #endif
#endif

/////////////////////////////////////////////////////

enum FStarPUTypes{
    // First will be zero
#ifdef STARPU_USE_CPU
    FSTARPU_CPU_IDX, // = 0
#endif
#ifdef STARPU_USE_CUDA
    FSTARPU_CUDA_IDX,
#endif
#ifdef STARPU_USE_OPENCL
    FSTARPU_OPENCL_IDX,
#endif
    // This will be the number of archs
    FSTARPU_NB_TYPES
};

const unsigned FStarPUTypesToArch[FSTARPU_NB_TYPES+1] = {
    #ifdef STARPU_USE_CPU
        STARPU_CPU,
    #endif
    #ifdef STARPU_USE_CUDA
        STARPU_CUDA,
    #endif
    #ifdef STARPU_USE_OPENCL
        STARPU_OPENCL,
    #endif
        0
};

inline FStarPUTypes FStarPUArchToTypes(const unsigned arch){
    switch(arch){
#ifdef STARPU_USE_CPU
    case STARPU_CPU: return FSTARPU_CPU_IDX;
#endif
#ifdef STARPU_USE_CUDA
    case STARPU_CUDA: return FSTARPU_CUDA_IDX;
#endif
#ifdef STARPU_USE_OPENCL
    case STARPU_OPENCL: return FSTARPU_OPENCL_IDX;
#endif
    default:;
    }
    return FSTARPU_NB_TYPES;
}

/////////////////////////////////////////////////////

#include <functional>

class FStarPUUtils{
protected:
    static void ExecOnWorkersBind(void* ptr){
        std::function<void(void)>* func = (std::function<void(void)>*) ptr;
        (*func)();
    }

public:
    static void ExecOnWorkers(const unsigned int inWorkersType, std::function<void(void)> func){
        starpu_execute_on_each_worker(ExecOnWorkersBind, &func, inWorkersType);
    }
};

/////////////////////////////////////////////////////

#ifndef STARPU_SUPPORT_COMMUTE
    #define STARPU_COMMUTE STARPU_NONE
#endif


/////////////////////////////////////////////////////

class FStarPUPtrInterface {
    void* ptrs[FSTARPU_NB_TYPES];

public:
    FStarPUPtrInterface(){
        memset(ptrs, 0, sizeof(void*)*FSTARPU_NB_TYPES);
    }

    void set(const FStarPUTypes idx, void* inPtr){
        ptrs[idx] = inPtr;
    }

    template <class PtrClass>
    PtrClass* get(const FStarPUTypes idx){
        return reinterpret_cast<PtrClass*>(ptrs[idx]);
    }
};

/////////////////////////////////////////////////////

#endif // FSTARPUUTILS_HPP

