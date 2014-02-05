#ifndef FP2P_HPP
#define FP2P_HPP

#include "../../Utils/FGlobal.hpp"
#include "../../Utils/FMath.hpp"

#ifdef ScalFMM_USE_SSE
#include "../../Utils/FSse.hpp"
#endif

/**
 * @brief The FP2P class
 */
class FP2P {
public:
    /**
     *
     */
#ifndef ScalFMM_USE_SSE
    template <class ContainerClass>
    static void FullMutual(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                           const int limiteNeighbors){

        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];
        FReal*const targetsForcesX = inTargets->getForcesX();
        FReal*const targetsForcesY = inTargets->getForcesY();
        FReal*const targetsForcesZ = inTargets->getForcesZ();
        FReal*const targetsPotentials = inTargets->getPotentials();

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
            if( inNeighbors[idxNeighbors] ){
                const int nbParticlesSources = inNeighbors[idxNeighbors]->getNbParticles();
                const FReal*const sourcesPhysicalValues = inNeighbors[idxNeighbors]->getPhysicalValues();
                const FReal*const sourcesX = inNeighbors[idxNeighbors]->getPositions()[0];
                const FReal*const sourcesY = inNeighbors[idxNeighbors]->getPositions()[1];
                const FReal*const sourcesZ = inNeighbors[idxNeighbors]->getPositions()[2];
                FReal*const sourcesForcesX = inNeighbors[idxNeighbors]->getForcesX();
                FReal*const sourcesForcesY = inNeighbors[idxNeighbors]->getForcesY();
                FReal*const sourcesForcesZ = inNeighbors[idxNeighbors]->getForcesZ();
                FReal*const sourcesPotentials = inNeighbors[idxNeighbors]->getPotentials();

                for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        FReal dx = sourcesX[idxSource] - targetsX[idxTarget];
                        FReal dy = sourcesY[idxSource] - targetsY[idxTarget];
                        FReal dz = sourcesZ[idxSource] - targetsZ[idxTarget];

                        FReal inv_square_distance = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                        const FReal inv_distance = FMath::Sqrt(inv_square_distance);

                        inv_square_distance *= inv_distance;
                        inv_square_distance *= targetsPhysicalValues[idxTarget] * sourcesPhysicalValues[idxSource];

                        dx *= inv_square_distance;
                        dy *= inv_square_distance;
                        dz *= inv_square_distance;

                        targetsForcesX[idxTarget] += dx;
                        targetsForcesY[idxTarget] += dy;
                        targetsForcesZ[idxTarget] += dz;
                        targetsPotentials[idxTarget] += inv_distance * sourcesPhysicalValues[idxSource];

                        sourcesForcesX[idxSource] -= dx;
                        sourcesForcesY[idxSource] -= dy;
                        sourcesForcesZ[idxSource] -= dz;
                        sourcesPotentials[idxSource] += inv_distance * targetsPhysicalValues[idxTarget];
                    }
                }
            }
        }

        for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
            for(int idxSource = idxTarget + 1 ; idxSource < nbParticlesTargets ; ++idxSource){
                FReal dx = targetsX[idxSource] - targetsX[idxTarget];
                FReal dy = targetsY[idxSource] - targetsY[idxTarget];
                FReal dz = targetsZ[idxSource] - targetsZ[idxTarget];

                FReal inv_square_distance = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                const FReal inv_distance = FMath::Sqrt(inv_square_distance);

                inv_square_distance *= inv_distance;
                inv_square_distance *= targetsPhysicalValues[idxTarget] * targetsPhysicalValues[idxSource];

                dx *= inv_square_distance;
                dy *= inv_square_distance;
                dz *= inv_square_distance;

                targetsForcesX[idxTarget] += dx;
                targetsForcesY[idxTarget] += dy;
                targetsForcesZ[idxTarget] += dz;
                targetsPotentials[idxTarget] += inv_distance * targetsPhysicalValues[idxSource];

                targetsForcesX[idxSource] -= dx;
                targetsForcesY[idxSource] -= dy;
                targetsForcesZ[idxSource] -= dz;
                targetsPotentials[idxSource] += inv_distance * targetsPhysicalValues[idxTarget];
            }
        }
    }
#else

#ifdef ScalFMM_USE_DOUBLE_PRECISION
    template <class ContainerClass>
    static void FullMutual(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                           const int limiteNeighbors){

        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];
        FReal*const targetsForcesX = inTargets->getForcesX();
        FReal*const targetsForcesY = inTargets->getForcesY();
        FReal*const targetsForcesZ = inTargets->getForcesZ();
        FReal*const targetsPotentials = inTargets->getPotentials();

        const __m128d mOne = _mm_set1_pd(1.0);

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
            if( inNeighbors[idxNeighbors] ){
                const int nbParticlesSources = (inNeighbors[idxNeighbors]->getNbParticles()+1)/2;
                const __m128d*const sourcesPhysicalValues = (const __m128d*)inNeighbors[idxNeighbors]->getPhysicalValues();
                const __m128d*const sourcesX = (const __m128d*)inNeighbors[idxNeighbors]->getPositions()[0];
                const __m128d*const sourcesY = (const __m128d*)inNeighbors[idxNeighbors]->getPositions()[1];
                const __m128d*const sourcesZ = (const __m128d*)inNeighbors[idxNeighbors]->getPositions()[2];
                __m128d*const sourcesForcesX = (__m128d*)inNeighbors[idxNeighbors]->getForcesX();
                __m128d*const sourcesForcesY = (__m128d*)inNeighbors[idxNeighbors]->getForcesY();
                __m128d*const sourcesForcesZ = (__m128d*)inNeighbors[idxNeighbors]->getForcesZ();
                __m128d*const sourcesPotentials = (__m128d*)inNeighbors[idxNeighbors]->getPotentials();

                for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                    const __m128d tx = _mm_load1_pd(&targetsX[idxTarget]);
                    const __m128d ty = _mm_load1_pd(&targetsY[idxTarget]);
                    const __m128d tz = _mm_load1_pd(&targetsZ[idxTarget]);
                    const __m128d tv = _mm_load1_pd(&targetsPhysicalValues[idxTarget]);
                    __m128d  tfx = _mm_setzero_pd();
                    __m128d  tfy = _mm_setzero_pd();
                    __m128d  tfz = _mm_setzero_pd();
                    __m128d  tpo = _mm_setzero_pd();

                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        __m128d dx = sourcesX[idxSource] - tx;
                        __m128d dy = sourcesY[idxSource] - ty;
                        __m128d dz = sourcesZ[idxSource] - tz;

                        __m128d inv_square_distance = mOne / (dx*dx + dy*dy + dz*dz);
                        const __m128d inv_distance = _mm_sqrt_pd(inv_square_distance);

                        inv_square_distance *= inv_distance;
                        inv_square_distance *= tv * sourcesPhysicalValues[idxSource];

                        dx *= inv_square_distance;
                        dy *= inv_square_distance;
                        dz *= inv_square_distance;

                        tfx += dx;
                        tfy += dy;
                        tfz += dz;
                        tpo += inv_distance * sourcesPhysicalValues[idxSource];

                        sourcesForcesX[idxSource] -= dx;
                        sourcesForcesY[idxSource] -= dy;
                        sourcesForcesZ[idxSource] -= dz;
                        sourcesPotentials[idxSource] += inv_distance * tv;
                    }

                    __attribute__((aligned(16))) double buffer[2];

                    _mm_store_pd(buffer, tfx);
                    targetsForcesX[idxTarget] += buffer[0] + buffer[1];

                    _mm_store_pd(buffer, tfy);
                    targetsForcesY[idxTarget] += buffer[0] + buffer[1];

                    _mm_store_pd(buffer, tfz);
                    targetsForcesZ[idxTarget] += buffer[0] + buffer[1];

                    _mm_store_pd(buffer, tpo);
                    targetsPotentials[idxTarget] += buffer[0] + buffer[1];
                }
            }
        }

        {
            const int nbParticlesSources = (nbParticlesTargets+1)/2;
            const __m128d*const sourcesPhysicalValues = (const __m128d*)targetsPhysicalValues;
            const __m128d*const sourcesX = (const __m128d*)targetsX;
            const __m128d*const sourcesY = (const __m128d*)targetsY;
            const __m128d*const sourcesZ = (const __m128d*)targetsZ;
            __m128d*const sourcesForcesX = (__m128d*)targetsForcesX;
            __m128d*const sourcesForcesY = (__m128d*)targetsForcesY;
            __m128d*const sourcesForcesZ = (__m128d*)targetsForcesZ;
            __m128d*const sourcesPotentials = (__m128d*)targetsPotentials;

            for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                const __m128d tx = _mm_load1_pd(&targetsX[idxTarget]);
                const __m128d ty = _mm_load1_pd(&targetsY[idxTarget]);
                const __m128d tz = _mm_load1_pd(&targetsZ[idxTarget]);
                const __m128d tv = _mm_load1_pd(&targetsPhysicalValues[idxTarget]);
                __m128d  tfx = _mm_setzero_pd();
                __m128d  tfy = _mm_setzero_pd();
                __m128d  tfz = _mm_setzero_pd();
                __m128d  tpo = _mm_setzero_pd();

                for(int idxSource = (idxTarget+2)/2 ; idxSource < nbParticlesSources ; ++idxSource){
                    __m128d dx = sourcesX[idxSource] - tx;
                    __m128d dy = sourcesY[idxSource] - ty;
                    __m128d dz = sourcesZ[idxSource] - tz;

                    __m128d inv_square_distance = mOne / (dx*dx + dy*dy + dz*dz);
                    const __m128d inv_distance = _mm_sqrt_pd(inv_square_distance);

                    inv_square_distance *= inv_distance;
                    inv_square_distance *= tv * sourcesPhysicalValues[idxSource];

                    dx *= inv_square_distance;
                    dy *= inv_square_distance;
                    dz *= inv_square_distance;

                    tfx += dx;
                    tfy += dy;
                    tfz += dz;
                    tpo += inv_distance * sourcesPhysicalValues[idxSource];

                    sourcesForcesX[idxSource] -= dx;
                    sourcesForcesY[idxSource] -= dy;
                    sourcesForcesZ[idxSource] -= dz;
                    sourcesPotentials[idxSource] += inv_distance * tv;
                }

                __attribute__((aligned(16))) double buffer[2];

                _mm_store_pd(buffer, tfx);
                targetsForcesX[idxTarget] += buffer[0] + buffer[1];

                _mm_store_pd(buffer, tfy);
                targetsForcesY[idxTarget] += buffer[0] + buffer[1];

                _mm_store_pd(buffer, tfz);
                targetsForcesZ[idxTarget] += buffer[0] + buffer[1];

                _mm_store_pd(buffer, tpo);
                targetsPotentials[idxTarget] += buffer[0] + buffer[1];
            }
        }

        for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; idxTarget += 2){
            const int idxSource = idxTarget + 1;
            FReal dx = targetsX[idxSource] - targetsX[idxTarget];
            FReal dy = targetsY[idxSource] - targetsY[idxTarget];
            FReal dz = targetsZ[idxSource] - targetsZ[idxTarget];

            FReal inv_square_distance = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
            const FReal inv_distance = FMath::Sqrt(inv_square_distance);

            inv_square_distance *= inv_distance;
            inv_square_distance *= targetsPhysicalValues[idxTarget] * targetsPhysicalValues[idxSource];

            dx *= inv_square_distance;
            dy *= inv_square_distance;
            dz *= inv_square_distance;

            targetsForcesX[idxTarget] += dx;
            targetsForcesY[idxTarget] += dy;
            targetsForcesZ[idxTarget] += dz;
            targetsPotentials[idxTarget] += inv_distance * targetsPhysicalValues[idxSource];

            targetsForcesX[idxSource] -= dx;
            targetsForcesY[idxSource] -= dy;
            targetsForcesZ[idxSource] -= dz;
            targetsPotentials[idxSource] += inv_distance * targetsPhysicalValues[idxTarget];
        }
    }
#else
    template <class ContainerClass>
    static void FullMutual(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                           const int limiteNeighbors){

        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];
        FReal*const targetsForcesX = inTargets->getForcesX();
        FReal*const targetsForcesY = inTargets->getForcesY();
        FReal*const targetsForcesZ = inTargets->getForcesZ();
        FReal*const targetsPotentials = inTargets->getPotentials();

        const __m128 mOne = _mm_set1_ps(1.0);

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
            if( inNeighbors[idxNeighbors] ){
                const int nbParticlesSources = (inNeighbors[idxNeighbors]->getNbParticles()+3)/4;
                const __m128*const sourcesPhysicalValues = (const __m128*)inNeighbors[idxNeighbors]->getPhysicalValues();
                const __m128*const sourcesX = (const __m128*)inNeighbors[idxNeighbors]->getPositions()[0];
                const __m128*const sourcesY = (const __m128*)inNeighbors[idxNeighbors]->getPositions()[1];
                const __m128*const sourcesZ = (const __m128*)inNeighbors[idxNeighbors]->getPositions()[2];
                __m128*const sourcesForcesX = (__m128*)inNeighbors[idxNeighbors]->getForcesX();
                __m128*const sourcesForcesY = (__m128*)inNeighbors[idxNeighbors]->getForcesY();
                __m128*const sourcesForcesZ = (__m128*)inNeighbors[idxNeighbors]->getForcesZ();
                __m128*const sourcesPotentials = (__m128*)inNeighbors[idxNeighbors]->getPotentials();

                for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                    const __m128 tx = _mm_load1_ps(&targetsX[idxTarget]);
                    const __m128 ty = _mm_load1_ps(&targetsY[idxTarget]);
                    const __m128 tz = _mm_load1_ps(&targetsZ[idxTarget]);
                    const __m128 tv = _mm_load1_ps(&targetsPhysicalValues[idxTarget]);
                    __m128  tfx = _mm_setzero_ps();
                    __m128  tfy = _mm_setzero_ps();
                    __m128  tfz = _mm_setzero_ps();
                    __m128  tpo = _mm_setzero_ps();

                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        __m128 dx = sourcesX[idxSource] - tx;
                        __m128 dy = sourcesY[idxSource] - ty;
                        __m128 dz = sourcesZ[idxSource] - tz;

                        __m128 inv_square_distance = mOne / (dx*dx + dy*dy + dz*dz);
                        const __m128 inv_distance = _mm_sqrt_ps(inv_square_distance);

                        inv_square_distance *= inv_distance;
                        inv_square_distance *= tv * sourcesPhysicalValues[idxSource];

                        dx *= inv_square_distance;
                        dy *= inv_square_distance;
                        dz *= inv_square_distance;

                        tfx += dx;
                        tfy += dy;
                        tfz += dz;
                        tpo += inv_distance * sourcesPhysicalValues[idxSource];

                        sourcesForcesX[idxSource] -= dx;
                        sourcesForcesY[idxSource] -= dy;
                        sourcesForcesZ[idxSource] -= dz;
                        sourcesPotentials[idxSource] += inv_distance * tv;
                    }

                    __attribute__((aligned(16))) float buffer[4];

                    _mm_store_ps(buffer, tfx);
                    targetsForcesX[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];

                    _mm_store_ps(buffer, tfy);
                    targetsForcesY[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];

                    _mm_store_ps(buffer, tfz);
                    targetsForcesZ[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];

                    _mm_store_ps(buffer, tpo);
                    targetsPotentials[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];
                }
            }
        }

        {
            const int nbParticlesSources = (nbParticlesTargets+3)/4;
            const __m128*const sourcesPhysicalValues = (const __m128*)targetsPhysicalValues;
            const __m128*const sourcesX = (const __m128*)targetsX;
            const __m128*const sourcesY = (const __m128*)targetsY;
            const __m128*const sourcesZ = (const __m128*)targetsZ;
            __m128*const sourcesForcesX = (__m128*)targetsForcesX;
            __m128*const sourcesForcesY = (__m128*)targetsForcesY;
            __m128*const sourcesForcesZ = (__m128*)targetsForcesZ;
            __m128*const sourcesPotentials = (__m128*)targetsPotentials;

            for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                const __m128 tx = _mm_load1_ps(&targetsX[idxTarget]);
                const __m128 ty = _mm_load1_ps(&targetsY[idxTarget]);
                const __m128 tz = _mm_load1_ps(&targetsZ[idxTarget]);
                const __m128 tv = _mm_load1_ps(&targetsPhysicalValues[idxTarget]);
                __m128  tfx = _mm_setzero_ps();
                __m128  tfy = _mm_setzero_ps();
                __m128  tfz = _mm_setzero_ps();
                __m128  tpo = _mm_setzero_ps();

                for(int idxSource = (idxTarget+2)/2 ; idxSource < nbParticlesSources ; ++idxSource){
                    __m128 dx = sourcesX[idxSource] - tx;
                    __m128 dy = sourcesY[idxSource] - ty;
                    __m128 dz = sourcesZ[idxSource] - tz;

                    __m128 inv_square_distance = mOne / (dx*dx + dy*dy + dz*dz);
                    const __m128 inv_distance = _mm_sqrt_ps(inv_square_distance);

                    inv_square_distance *= inv_distance;
                    inv_square_distance *= tv * sourcesPhysicalValues[idxSource];

                    dx *= inv_square_distance;
                    dy *= inv_square_distance;
                    dz *= inv_square_distance;

                    tfx += dx;
                    tfy += dy;
                    tfz += dz;
                    tpo += inv_distance * sourcesPhysicalValues[idxSource];

                    sourcesForcesX[idxSource] -= dx;
                    sourcesForcesY[idxSource] -= dy;
                    sourcesForcesZ[idxSource] -= dz;
                    sourcesPotentials[idxSource] += inv_distance * tv;
                }

                __attribute__((aligned(16))) float buffer[4];

                _mm_store_ps(buffer, tfx);
                targetsForcesX[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];

                _mm_store_ps(buffer, tfy);
                targetsForcesY[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];

                _mm_store_ps(buffer, tfz);
                targetsForcesZ[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];

                _mm_store_ps(buffer, tpo);
                targetsPotentials[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];
            }
        }

        for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; idxTarget += 4){
            for(int idxClose = 1 ; idxClose < 4; ++idxClose){
                const int idxSource = idxTarget + idxClose;
                FReal dx = targetsX[idxSource] - targetsX[idxTarget];
                FReal dy = targetsY[idxSource] - targetsY[idxTarget];
                FReal dz = targetsZ[idxSource] - targetsZ[idxTarget];

                FReal inv_square_distance = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                const FReal inv_distance = FMath::Sqrt(inv_square_distance);

                inv_square_distance *= inv_distance;
                inv_square_distance *= targetsPhysicalValues[idxTarget] * targetsPhysicalValues[idxSource];

                dx *= inv_square_distance;
                dy *= inv_square_distance;
                dz *= inv_square_distance;

                targetsForcesX[idxTarget] += dx;
                targetsForcesY[idxTarget] += dy;
                targetsForcesZ[idxTarget] += dz;
                targetsPotentials[idxTarget] += inv_distance * targetsPhysicalValues[idxSource];

                targetsForcesX[idxSource] -= dx;
                targetsForcesY[idxSource] -= dy;
                targetsForcesZ[idxSource] -= dz;
                targetsPotentials[idxSource] += inv_distance * targetsPhysicalValues[idxTarget];
            }
        }
    }
#endif

#endif

    /**
     *
     */
#ifndef ScalFMM_USE_SSE
    template <class ContainerClass>
    static void FullRemote(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                           const int limiteNeighbors){
        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];
        FReal*const targetsForcesX = inTargets->getForcesX();
        FReal*const targetsForcesY = inTargets->getForcesY();
        FReal*const targetsForcesZ = inTargets->getForcesZ();
        FReal*const targetsPotentials = inTargets->getPotentials();

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
            if( inNeighbors[idxNeighbors] ){
                const int nbParticlesSources = inNeighbors[idxNeighbors]->getNbParticles();
                const FReal*const sourcesPhysicalValues = inNeighbors[idxNeighbors]->getPhysicalValues();
                const FReal*const sourcesX = inNeighbors[idxNeighbors]->getPositions()[0];
                const FReal*const sourcesY = inNeighbors[idxNeighbors]->getPositions()[1];
                const FReal*const sourcesZ = inNeighbors[idxNeighbors]->getPositions()[2];

                for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){

                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        FReal dx = sourcesX[idxSource] - targetsX[idxTarget];
                        FReal dy = sourcesY[idxSource] - targetsY[idxTarget];
                        FReal dz = sourcesZ[idxSource] - targetsZ[idxTarget];

                        FReal inv_square_distance = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                        const FReal inv_distance = FMath::Sqrt(inv_square_distance);

                        inv_square_distance *= inv_distance;
                        inv_square_distance *= targetsPhysicalValues[idxTarget] * sourcesPhysicalValues[idxSource];

                        dx *= inv_square_distance;
                        dy *= inv_square_distance;
                        dz *= inv_square_distance;

                        targetsForcesX[idxTarget] += dx;
                        targetsForcesY[idxTarget] += dy;
                        targetsForcesZ[idxTarget] += dz;
                        targetsPotentials[idxTarget] += inv_distance * sourcesPhysicalValues[idxSource];
                    }
                }
            }
        }
    }
#else

#ifdef ScalFMM_USE_DOUBLE_PRECISION
    template <class ContainerClass>
    static void FullRemote(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                           const int limiteNeighbors){
        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];
        FReal*const targetsForcesX = inTargets->getForcesX();
        FReal*const targetsForcesY = inTargets->getForcesY();
        FReal*const targetsForcesZ = inTargets->getForcesZ();
        FReal*const targetsPotentials = inTargets->getPotentials();

        const __m128d mOne = _mm_set1_pd(1.0);

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
            if( inNeighbors[idxNeighbors] ){
                const int nbParticlesSources = (inNeighbors[idxNeighbors]->getNbParticles()+1)/2;
                const __m128d*const sourcesPhysicalValues = (const __m128d*)inNeighbors[idxNeighbors]->getPhysicalValues();
                const __m128d*const sourcesX = (const __m128d*)inNeighbors[idxNeighbors]->getPositions()[0];
                const __m128d*const sourcesY = (const __m128d*)inNeighbors[idxNeighbors]->getPositions()[1];
                const __m128d*const sourcesZ = (const __m128d*)inNeighbors[idxNeighbors]->getPositions()[2];

                for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                    const __m128d tx = _mm_load1_pd(&targetsX[idxTarget]);
                    const __m128d ty = _mm_load1_pd(&targetsY[idxTarget]);
                    const __m128d tz = _mm_load1_pd(&targetsZ[idxTarget]);
                    const __m128d tv = _mm_load1_pd(&targetsPhysicalValues[idxTarget]);
                    __m128d  tfx = _mm_setzero_pd();
                    __m128d  tfy = _mm_setzero_pd();
                    __m128d  tfz = _mm_setzero_pd();
                    __m128d  tpo = _mm_setzero_pd();

                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        __m128d dx = sourcesX[idxSource] - tx;
                        __m128d dy = sourcesY[idxSource] - ty;
                        __m128d dz = sourcesZ[idxSource] - tz;

                        __m128d inv_square_distance = mOne / (dx*dx + dy*dy + dz*dz);
                        const __m128d inv_distance = _mm_sqrt_pd(inv_square_distance);

                        inv_square_distance *= inv_distance;
                        inv_square_distance *= tv * sourcesPhysicalValues[idxSource];

                        dx *= inv_square_distance;
                        dy *= inv_square_distance;
                        dz *= inv_square_distance;

                        tfx += dx;
                        tfy += dy;
                        tfz += dz;
                        tpo += inv_distance * sourcesPhysicalValues[idxSource];
                    }

                    __attribute__((aligned(16))) double buffer[2];

                    _mm_store_pd(buffer, tfx);
                    targetsForcesX[idxTarget] += buffer[0] + buffer[1];

                    _mm_store_pd(buffer, tfy);
                    targetsForcesY[idxTarget] += buffer[0] + buffer[1];

                    _mm_store_pd(buffer, tfz);
                    targetsForcesZ[idxTarget] += buffer[0] + buffer[1];

                    _mm_store_pd(buffer, tpo);
                    targetsPotentials[idxTarget] += buffer[0] + buffer[1];
                }
            }
        }
    }
#else
    template <class ContainerClass>
    static void FullRemote(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                           const int limiteNeighbors){
        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];
        FReal*const targetsForcesX = inTargets->getForcesX();
        FReal*const targetsForcesY = inTargets->getForcesY();
        FReal*const targetsForcesZ = inTargets->getForcesZ();
        FReal*const targetsPotentials = inTargets->getPotentials();

        const __m128 mOne = _mm_set1_ps(1.0);

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
            if( inNeighbors[idxNeighbors] ){
                const int nbParticlesSources = (inNeighbors[idxNeighbors]->getNbParticles()+3)/4;
                const __m128*const sourcesPhysicalValues = (const __m128*)inNeighbors[idxNeighbors]->getPhysicalValues();
                const __m128*const sourcesX = (const __m128*)inNeighbors[idxNeighbors]->getPositions()[0];
                const __m128*const sourcesY = (const __m128*)inNeighbors[idxNeighbors]->getPositions()[1];
                const __m128*const sourcesZ = (const __m128*)inNeighbors[idxNeighbors]->getPositions()[2];

                for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                    const __m128 tx = _mm_load1_ps(&targetsX[idxTarget]);
                    const __m128 ty = _mm_load1_ps(&targetsY[idxTarget]);
                    const __m128 tz = _mm_load1_ps(&targetsZ[idxTarget]);
                    const __m128 tv = _mm_load1_ps(&targetsPhysicalValues[idxTarget]);
                    __m128  tfx = _mm_setzero_ps();
                    __m128  tfy = _mm_setzero_ps();
                    __m128  tfz = _mm_setzero_ps();
                    __m128  tpo = _mm_setzero_ps();

                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        __m128 dx = sourcesX[idxSource] - tx;
                        __m128 dy = sourcesY[idxSource] - ty;
                        __m128 dz = sourcesZ[idxSource] - tz;

                        __m128 inv_square_distance = mOne / (dx*dx + dy*dy + dz*dz);
                        const __m128 inv_distance = _mm_sqrt_ps(inv_square_distance);

                        inv_square_distance *= inv_distance;
                        inv_square_distance *= tv * sourcesPhysicalValues[idxSource];

                        dx *= inv_square_distance;
                        dy *= inv_square_distance;
                        dz *= inv_square_distance;

                        tfx += dx;
                        tfy += dy;
                        tfz += dz;
                        tpo += inv_distance * sourcesPhysicalValues[idxSource];
                    }

                    __attribute__((aligned(16))) float buffer[4];

                    _mm_store_ps(buffer, tfx);
                    targetsForcesX[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];

                    _mm_store_ps(buffer, tfy);
                    targetsForcesY[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];

                    _mm_store_ps(buffer, tfz);
                    targetsForcesZ[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];

                    _mm_store_ps(buffer, tpo);
                    targetsPotentials[idxTarget] += buffer[0] + buffer[1] + buffer[2] + buffer[3];
                }
            }
        }
    }
#endif

#endif

    /** P2P mutual interaction,
      * this function computes the interaction for 2 particles.
      *
      * Formulas are:
      * \f[
      * F = q_1 * q_2 / r^2
      * P_1 = q_2 / r ; P_2 = q_1 / r
      * \f]
      * In details :
      * \f$ F(x) = \frac{ \Delta_x * q_1 * q_2 }{ r^2 } = \Delta_x * F \f$
      */
    static void MutualParticles(const FReal sourceX,const FReal sourceY,const FReal sourceZ, const FReal sourcePhysicalValue,
                                FReal* sourceForceX, FReal* sourceForceY, FReal* sourceForceZ, FReal* sourcePotential,
                                const FReal targetX,const FReal targetY,const FReal targetZ, const FReal targetPhysicalValue,
                                FReal* targetForceX, FReal* targetForceY, FReal* targetForceZ, FReal* targetPotential
                                ){
        FReal dx = sourceX - targetX;
        FReal dy = sourceY - targetY;
        FReal dz = sourceZ - targetZ;

        FReal inv_square_distance = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
        FReal inv_distance = FMath::Sqrt(inv_square_distance);

        inv_square_distance *= inv_distance;
        inv_square_distance *= targetPhysicalValue * sourcePhysicalValue;

        dx *= inv_square_distance;
        dy *= inv_square_distance;
        dz *= inv_square_distance;

        *targetForceX += dx;
        *targetForceY += dy;
        *targetForceZ += dz;
        *targetPotential += ( inv_distance * sourcePhysicalValue );

        *sourceForceX -= dx;
        *sourceForceY -= dy;
        *sourceForceZ -= dz;
        *sourcePotential += ( inv_distance * targetPhysicalValue );
    }

    /** P2P mutual interaction,
      * this function computes the interaction for 2 particles.
      *
      * Formulas are:
      * \f[
      * F = q_1 * q_2 / r^2
      * P_1 = q_2 / r ; P_2 = q_1 / r
      * \f]
      * In details :
      * \f$ F(x) = \frac{ \Delta_x * q_1 * q_2 }{ r^2 } = \Delta_x * F \f$
      */
    static void NonMutualParticles(const FReal sourceX,const FReal sourceY,const FReal sourceZ, const FReal sourcePhysicalValue,
                                   const FReal targetX,const FReal targetY,const FReal targetZ, const FReal targetPhysicalValue,
                                   FReal* targetForceX, FReal* targetForceY, FReal* targetForceZ, FReal* targetPotential){
        FReal dx = sourceX - targetX;
        FReal dy = sourceY - targetY;
        FReal dz = sourceZ - targetZ;

        FReal inv_square_distance = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
        FReal inv_distance = FMath::Sqrt(inv_square_distance);

        inv_square_distance *= inv_distance;
        inv_square_distance *= targetPhysicalValue * sourcePhysicalValue;

        dx *= inv_square_distance;
        dy *= inv_square_distance;
        dz *= inv_square_distance;

        *targetForceX += dx;
        *targetForceY += dy;
        *targetForceZ += dz;
        *targetPotential += ( inv_distance * sourcePhysicalValue );
    }

    template <class ContainerClass>
    static void FullMutualLJ(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                             const int limiteNeighbors){

        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];
        FReal*const targetsForcesX = inTargets->getForcesX();
        FReal*const targetsForcesY = inTargets->getForcesY();
        FReal*const targetsForcesZ = inTargets->getForcesZ();
        FReal*const targetsPotentials = inTargets->getPotentials();

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
            for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                if( inNeighbors[idxNeighbors] ){
                    const int nbParticlesSources = inNeighbors[idxNeighbors]->getNbParticles();
                    const FReal*const sourcesPhysicalValues = inNeighbors[idxNeighbors]->getPhysicalValues();
                    const FReal*const sourcesX = inNeighbors[idxNeighbors]->getPositions()[0];
                    const FReal*const sourcesY = inNeighbors[idxNeighbors]->getPositions()[1];
                    const FReal*const sourcesZ = inNeighbors[idxNeighbors]->getPositions()[2];
                    FReal*const sourcesForcesX = inNeighbors[idxNeighbors]->getForcesX();
                    FReal*const sourcesForcesY = inNeighbors[idxNeighbors]->getForcesY();
                    FReal*const sourcesForcesZ = inNeighbors[idxNeighbors]->getForcesZ();
                    FReal*const sourcesPotentials = inNeighbors[idxNeighbors]->getPotentials();

                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        FReal dx = sourcesX[idxSource] - targetsX[idxTarget];
                        FReal dy = sourcesY[idxSource] - targetsY[idxTarget];
                        FReal dz = sourcesZ[idxSource] - targetsZ[idxTarget];

                        FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                        FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
                        FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;
                        FReal inv_distance_pow6 = inv_distance_pow3 * inv_distance_pow3;
                        FReal inv_distance_pow8 = inv_distance_pow6 * inv_distance_pow2;

                        FReal coef = ((targetsPhysicalValues[idxTarget] * sourcesPhysicalValues[idxSource])
                                      * (FReal(12.0)*inv_distance_pow6*inv_distance_pow8 - FReal(6.0)*inv_distance_pow8));
                        FReal potentialCoef = (inv_distance_pow6*inv_distance_pow6-inv_distance_pow6);

                        dx *= coef;
                        dy *= coef;
                        dz *= coef;

                        targetsForcesX[idxTarget] += dx;
                        targetsForcesY[idxTarget] += dy;
                        targetsForcesZ[idxTarget] += dz;
                        targetsPotentials[idxTarget] += ( potentialCoef * sourcesPhysicalValues[idxSource] );

                        sourcesForcesX[idxSource] -= dx;
                        sourcesForcesY[idxSource] -= dy;
                        sourcesForcesZ[idxSource] -= dz;
                        sourcesPotentials[idxSource] += potentialCoef * targetsPhysicalValues[idxTarget];
                    }
                }
            }
        }

        for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
            for(int idxSource = idxTarget + 1 ; idxSource < nbParticlesTargets ; ++idxSource){
                FReal dx = targetsX[idxSource] - targetsX[idxTarget];
                FReal dy = targetsY[idxSource] - targetsY[idxTarget];
                FReal dz = targetsZ[idxSource] - targetsZ[idxTarget];

                FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
                FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;
                FReal inv_distance_pow6 = inv_distance_pow3 * inv_distance_pow3;
                FReal inv_distance_pow8 = inv_distance_pow6 * inv_distance_pow2;

                FReal coef = ((targetsPhysicalValues[idxTarget] * targetsPhysicalValues[idxSource])
                              * (FReal(12.0)*inv_distance_pow6*inv_distance_pow8 - FReal(6.0)*inv_distance_pow8));
                FReal potentialCoef = (inv_distance_pow6*inv_distance_pow6-inv_distance_pow6);

                dx *= coef;
                dy *= coef;
                dz *= coef;

                targetsForcesX[idxTarget] += dx;
                targetsForcesY[idxTarget] += dy;
                targetsForcesZ[idxTarget] += dz;
                targetsPotentials[idxTarget] += ( potentialCoef * targetsPhysicalValues[idxSource] );

                targetsForcesX[idxSource] -= dx;
                targetsForcesY[idxSource] -= dy;
                targetsForcesZ[idxSource] -= dz;
                targetsPotentials[idxSource] += potentialCoef * targetsPhysicalValues[idxTarget];
            }
        }
    }

    /**
     *
     */
    template <class ContainerClass>
    static void FullRemoteLJ(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                             const int limiteNeighbors){
        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];
        FReal*const targetsForcesX = inTargets->getForcesX();
        FReal*const targetsForcesY = inTargets->getForcesY();
        FReal*const targetsForcesZ = inTargets->getForcesZ();
        FReal*const targetsPotentials = inTargets->getPotentials();

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
            for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                if( inNeighbors[idxNeighbors] ){
                    const int nbParticlesSources = inNeighbors[idxNeighbors]->getNbParticles();
                    const FReal*const sourcesPhysicalValues = inNeighbors[idxNeighbors]->getPhysicalValues();
                    const FReal*const sourcesX = inNeighbors[idxNeighbors]->getPositions()[0];
                    const FReal*const sourcesY = inNeighbors[idxNeighbors]->getPositions()[1];
                    const FReal*const sourcesZ = inNeighbors[idxNeighbors]->getPositions()[2];

                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        // lenard-jones potential
                        FReal dx = sourcesX[idxSource] - targetsX[idxTarget];
                        FReal dy = sourcesY[idxSource] - targetsY[idxTarget];
                        FReal dz = sourcesZ[idxSource] - targetsZ[idxTarget];

                        FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                        FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
                        FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;
                        FReal inv_distance_pow6 = inv_distance_pow3 * inv_distance_pow3;
                        FReal inv_distance_pow8 = inv_distance_pow6 * inv_distance_pow2;

                        FReal coef = ((targetsPhysicalValues[idxTarget] * sourcesPhysicalValues[idxSource])
                                      * (FReal(12.0)*inv_distance_pow6*inv_distance_pow8 - FReal(6.0)*inv_distance_pow8));

                        dx *= coef;
                        dy *= coef;
                        dz *= coef;

                        targetsForcesX[idxTarget] += dx;
                        targetsForcesY[idxTarget] += dy;
                        targetsForcesZ[idxTarget] += dz;
                        targetsPotentials[idxTarget] += ( (inv_distance_pow6*inv_distance_pow6-inv_distance_pow6) * sourcesPhysicalValues[idxSource] );
                    }
                }
            }
        }
    }


    /**
     * @brief NonMutualParticlesLJ
     * @param sourceX
     * @param sourceY
     * @param sourceZ
     * @param sourcePhysicalValue
     * @param targetX
     * @param targetY
     * @param targetZ
     * @param targetPhysicalValue
     * @param targetForceX
     * @param targetForceY
     * @param targetForceZ
     * @param targetPotential
     */
    static void NonMutualParticlesLJ(const FReal sourceX,const FReal sourceY,const FReal sourceZ, const FReal sourcePhysicalValue,
                                     const FReal targetX,const FReal targetY,const FReal targetZ, const FReal targetPhysicalValue,
                                     FReal* targetForceX, FReal* targetForceY, FReal* targetForceZ, FReal* targetPotential){
        // lenard-jones potential
        FReal dx = sourceX - targetX;
        FReal dy = sourceY - targetY;
        FReal dz = sourceZ - targetZ;

        FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
        FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
        FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;
        FReal inv_distance_pow6 = inv_distance_pow3 * inv_distance_pow3;
        FReal inv_distance_pow8 = inv_distance_pow6 * inv_distance_pow2;

        FReal coef = ((targetPhysicalValue * sourcePhysicalValue) * (FReal(12.0)*inv_distance_pow6*inv_distance_pow8
                                                                     - FReal(6.0)*inv_distance_pow8));

        dx *= coef;
        dy *= coef;
        dz *= coef;

        (*targetForceX) += dx;
        (*targetForceY) += dy;
        (*targetForceZ) += dz;
        (*targetPotential) += ( (inv_distance_pow6*inv_distance_pow6-inv_distance_pow6) * sourcePhysicalValue );
    }

    /**
     * @brief MutualParticlesLJ
     * @param sourceX
     * @param sourceY
     * @param sourceZ
     * @param sourcePhysicalValue
     * @param sourceForceX
     * @param sourceForceY
     * @param sourceForceZ
     * @param sourcePotential
     * @param targetX
     * @param targetY
     * @param targetZ
     * @param targetPhysicalValue
     * @param targetForceX
     * @param targetForceY
     * @param targetForceZ
     * @param targetPotential
     */
    static void MutualParticlesLJ(const FReal sourceX,const FReal sourceY,const FReal sourceZ, const FReal sourcePhysicalValue,
                                  FReal* sourceForceX, FReal* sourceForceY, FReal* sourceForceZ, FReal* sourcePotential,
                                  const FReal targetX,const FReal targetY,const FReal targetZ, const FReal targetPhysicalValue,
                                  FReal* targetForceX, FReal* targetForceY, FReal* targetForceZ, FReal* targetPotential
                                  ){
        // lenard-jones potential
        FReal dx = sourceX - targetX;
        FReal dy = sourceY - targetY;
        FReal dz = sourceZ - targetZ;

        FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
        FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
        FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;
        FReal inv_distance_pow6 = inv_distance_pow3 * inv_distance_pow3;
        FReal inv_distance_pow8 = inv_distance_pow6 * inv_distance_pow2;

        FReal coef = ((targetPhysicalValue * sourcePhysicalValue) * (FReal(12.0)*inv_distance_pow6*inv_distance_pow8
                                                                     - FReal(6.0)*inv_distance_pow8));
        FReal potentialCoef = (inv_distance_pow6*inv_distance_pow6-inv_distance_pow6);

        dx *= coef;
        dy *= coef;
        dz *= coef;

        (*targetForceX) += dx;
        (*targetForceY) += dy;
        (*targetForceZ) += dz;
        (*targetPotential) += ( potentialCoef * sourcePhysicalValue );

        (*sourceForceX) -= dx;
        (*sourceForceY) -= dy;
        (*sourceForceZ) -= dz;
        (*sourcePotential) += ( potentialCoef * targetPhysicalValue );
    }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // R_IJ
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief FullMutualRIJ
     */
    template <class ContainerClass>
    static void FullMutualRIJ(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                             const int limiteNeighbors){

        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
            for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                if( inNeighbors[idxNeighbors] ){
                    const int nbParticlesSources = inNeighbors[idxNeighbors]->getNbParticles();
                    const FReal*const sourcesX = inNeighbors[idxNeighbors]->getPositions()[0];
                    const FReal*const sourcesY = inNeighbors[idxNeighbors]->getPositions()[1];
                    const FReal*const sourcesZ = inNeighbors[idxNeighbors]->getPositions()[2];

                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        FReal dx = sourcesX[idxSource] - targetsX[idxTarget];
                        FReal dy = sourcesY[idxSource] - targetsY[idxTarget];
                        FReal dz = sourcesZ[idxSource] - targetsZ[idxTarget];

                        FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                        FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
                        FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;

                        FReal r[3]={dx,dy,dz};

                        for(unsigned int i = 0 ; i < 3 ; ++i){
                          FReal*const targetsPotentials = inTargets->getPotentials(i);
                          FReal*const targetsForcesX = inTargets->getForcesX(i);
                          FReal*const targetsForcesY = inTargets->getForcesY(i);
                          FReal*const targetsForcesZ = inTargets->getForcesZ(i);
                          FReal*const sourcesPotentials = inNeighbors[idxNeighbors]->getPotentials(i);
                          FReal*const sourcesForcesX = inNeighbors[idxNeighbors]->getForcesX(i);
                          FReal*const sourcesForcesY = inNeighbors[idxNeighbors]->getForcesY(i);
                          FReal*const sourcesForcesZ = inNeighbors[idxNeighbors]->getForcesZ(i);

                          FReal ri2=r[i]*r[i];

                          for(unsigned int j = 0 ; j < 3 ; ++j){
                            const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues(j);
                            const FReal*const sourcesPhysicalValues = inNeighbors[idxNeighbors]->getPhysicalValues(j);

                            // potentials
                            FReal potentialCoef;
                            if(i==j)
                              potentialCoef = inv_distance - ri2 * inv_distance_pow3;
                            else
                              potentialCoef = - r[i] * r[j] * inv_distance_pow3;

                            // forces
                            FReal rj2=r[j]*r[j];

                            FReal coef[3]; 
                            for(unsigned int k = 0 ; k < 3 ; ++k)
                              coef[k]= -(targetsPhysicalValues[idxTarget] * sourcesPhysicalValues[idxSource]);

                            // Grad of RIJ kernel is RIJK kernel => use same expression as in FInterpMatrixKernel
                            for(unsigned int k = 0 ; k < 3 ; ++k){
                              if(i==j){
                                if(j==k) //i=j=k
                                  coef[k] *= FReal(3.) * ( FReal(-1.) + ri2 * inv_distance_pow2 ) * r[i] * inv_distance_pow3;
                                else //i=j!=k
                                  coef[k] *= ( FReal(-1.) + FReal(3.) * ri2 * inv_distance_pow2 ) * r[k] * inv_distance_pow3;
                              }
                              else{ //(i!=j)
                                if(i==k) //i=k!=j
                                  coef[k] *= ( FReal(-1.) + FReal(3.) * ri2 * inv_distance_pow2 ) * r[j] * inv_distance_pow3;
                                else if(j==k) //i!=k=j
                                  coef[k] *= ( FReal(-1.) + FReal(3.) * rj2 * inv_distance_pow2 ) * r[i] * inv_distance_pow3;
                                else //i!=k!=j
                                  coef[k] *= FReal(3.) * r[i] * r[j] * r[k] * inv_distance_pow2 * inv_distance_pow3;
                              }
                            }// k

                            targetsForcesX[idxTarget] += coef[0];
                            targetsForcesY[idxTarget] += coef[1];
                            targetsForcesZ[idxTarget] += coef[2];
                            targetsPotentials[idxTarget] += ( potentialCoef * sourcesPhysicalValues[idxSource] );

                            sourcesForcesX[idxSource] -= coef[0];
                            sourcesForcesY[idxSource] -= coef[1];
                            sourcesForcesZ[idxSource] -= coef[2];
                            sourcesPotentials[idxSource] += potentialCoef * targetsPhysicalValues[idxTarget];

                             
                          }// j
                        }// i
                    }
                }
            }
        }

        for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
            for(int idxSource = idxTarget + 1 ; idxSource < nbParticlesTargets ; ++idxSource){
                FReal dx = targetsX[idxSource] - targetsX[idxTarget];
                FReal dy = targetsY[idxSource] - targetsY[idxTarget];
                FReal dz = targetsZ[idxSource] - targetsZ[idxTarget];

                FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
                FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;

                FReal r[3]={dx,dy,dz};

                for(unsigned int i = 0 ; i < 3 ; ++i){
                  FReal*const targetsPotentials = inTargets->getPotentials(i);
                  FReal*const targetsForcesX = inTargets->getForcesX(i);
                  FReal*const targetsForcesY = inTargets->getForcesY(i);
                  FReal*const targetsForcesZ = inTargets->getForcesZ(i);
                  FReal ri2=r[i]*r[i];

                  for(unsigned int j = 0 ; j < 3 ; ++j){
                    const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues(j);

                    // potentials
                    FReal potentialCoef;
                    if(i==j)
                      potentialCoef = inv_distance - ri2 * inv_distance_pow3;
                    else
                      potentialCoef = - r[i] * r[j] * inv_distance_pow3;

                    // forces
                    FReal rj2=r[j]*r[j];

                    FReal coef[3]; 
                    for(unsigned int k = 0 ; k < 3 ; ++k)
                      coef[k]= -(targetsPhysicalValues[idxTarget] * targetsPhysicalValues[idxSource]);

                    // Grad of RIJ kernel is RIJK kernel => use same expression as in FInterpMatrixKernel
                    for(unsigned int k = 0 ; k < 3 ; ++k){
                      if(i==j){
                        if(j==k) //i=j=k
                          coef[k] *= FReal(3.) * ( FReal(-1.) + ri2 * inv_distance_pow2 ) * r[i] * inv_distance_pow3;
                        else //i=j!=k
                          coef[k] *= ( FReal(-1.) + FReal(3.) * ri2 * inv_distance_pow2 ) * r[k] * inv_distance_pow3;
                      }
                      else{ //(i!=j)
                        if(i==k) //i=k!=j
                          coef[k] *= ( FReal(-1.) + FReal(3.) * ri2 * inv_distance_pow2 ) * r[j] * inv_distance_pow3;
                        else if(j==k) //i!=k=j
                          coef[k] *= ( FReal(-1.) + FReal(3.) * rj2 * inv_distance_pow2 ) * r[i] * inv_distance_pow3;
                        else //i!=k!=j
                          coef[k] *= FReal(3.) * r[i] * r[j] * r[k] * inv_distance_pow2 * inv_distance_pow3;
                      }
                    }// k


                    targetsForcesX[idxTarget] += coef[0];
                    targetsForcesY[idxTarget] += coef[1];
                    targetsForcesZ[idxTarget] += coef[2];
                    targetsPotentials[idxTarget] += ( potentialCoef * targetsPhysicalValues[idxSource] );

                    targetsForcesX[idxSource] -= coef[0];
                    targetsForcesY[idxSource] -= coef[1];
                    targetsForcesZ[idxSource] -= coef[2];
                    targetsPotentials[idxSource] += potentialCoef * targetsPhysicalValues[idxTarget];
                             
                  }// j
                }// i

            }
        }
    }

    /**
     * @brief FullRemoteRIJ
     */
    template <class ContainerClass>
    static void FullRemoteRIJ(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                             const int limiteNeighbors){

        const int nbParticlesTargets = inTargets->getNbParticles();
        const FReal*const targetsX = inTargets->getPositions()[0];
        const FReal*const targetsY = inTargets->getPositions()[1];
        const FReal*const targetsZ = inTargets->getPositions()[2];

        for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
          for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
                if( inNeighbors[idxNeighbors] ){
                    const int nbParticlesSources = inNeighbors[idxNeighbors]->getNbParticles();
                    const FReal*const sourcesX = inNeighbors[idxNeighbors]->getPositions()[0];
                    const FReal*const sourcesY = inNeighbors[idxNeighbors]->getPositions()[1];
                    const FReal*const sourcesZ = inNeighbors[idxNeighbors]->getPositions()[2];

                    for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
                        // potential
                        FReal dx = sourcesX[idxSource] - targetsX[idxTarget];
                        FReal dy = sourcesY[idxSource] - targetsY[idxTarget];
                        FReal dz = sourcesZ[idxSource] - targetsZ[idxTarget];

                        FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
                        FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
                        FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;

                        FReal r[3]={dx,dy,dz};

                        for(unsigned int i = 0 ; i < 3 ; ++i){
                          FReal*const targetsPotentials = inTargets->getPotentials(i);
                          FReal*const targetsForcesX = inTargets->getForcesX(i);
                          FReal*const targetsForcesY = inTargets->getForcesY(i);
                          FReal*const targetsForcesZ = inTargets->getForcesZ(i);
                          FReal ri2=r[i]*r[i];

                          for(unsigned int j = 0 ; j < 3 ; ++j){
                            const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues(j);
                            const FReal*const sourcesPhysicalValues = inNeighbors[idxNeighbors]->getPhysicalValues(j);

                            // potentials
                            FReal potentialCoef;
                            if(i==j)
                              potentialCoef = inv_distance - ri2 * inv_distance_pow3;
                            else
                              potentialCoef = - r[i] * r[j] * inv_distance_pow3;

                            // forces
                            FReal rj2=r[j]*r[j];

                            FReal coef[3]; 
                            for(unsigned int k = 0 ; k < 3 ; ++k)
                              coef[k]= -(targetsPhysicalValues[idxTarget] * sourcesPhysicalValues[idxSource]);

                            // Grad of RIJ kernel is RIJK kernel => use same expression as in FInterpMatrixKernel
                            for(unsigned int k = 0 ; k < 3 ; ++k){
                              if(i==j){
                                if(j==k) //i=j=k
                                  coef[k] *= FReal(3.) * ( FReal(-1.) + ri2 * inv_distance_pow2 ) * r[i] * inv_distance_pow3;
                                else //i=j!=k
                                  coef[k] *= ( FReal(-1.) + FReal(3.) * ri2 * inv_distance_pow2 ) * r[k] * inv_distance_pow3;
                              }
                              else{ //(i!=j)
                                if(i==k) //i=k!=j
                                  coef[k] *= ( FReal(-1.) + FReal(3.) * ri2 * inv_distance_pow2 ) * r[j] * inv_distance_pow3;
                                else if(j==k) //i!=k=j
                                  coef[k] *= ( FReal(-1.) + FReal(3.) * rj2 * inv_distance_pow2 ) * r[i] * inv_distance_pow3;
                                else //i!=k!=j
                                  coef[k] *= FReal(3.) * r[i] * r[j] * r[k] * inv_distance_pow2 * inv_distance_pow3;
                              }
                            }// k

                            targetsForcesX[idxTarget] += coef[0];
                            targetsForcesY[idxTarget] += coef[1];
                            targetsForcesZ[idxTarget] += coef[2];
                            targetsPotentials[idxTarget] += ( potentialCoef * sourcesPhysicalValues[idxSource] );

                          }// j
                        }// i

                    }
                }
            }
        }
    }


    /**
     * @brief MutualParticlesRIJ
     * @param sourceX
     * @param sourceY
     * @param sourceZ
     * @param sourcePhysicalValue
     * @param sourceForceX
     * @param sourceForceY
     * @param sourceForceZ
     * @param sourcePotential
     * @param targetX
     * @param targetY
     * @param targetZ
     * @param targetPhysicalValue
     * @param targetForceX
     * @param targetForceY
     * @param targetForceZ
     * @param targetPotential
     */
    static void MutualParticlesRIJ(const FReal sourceX,const FReal sourceY,const FReal sourceZ, const FReal* sourcePhysicalValue,
                                  FReal* sourceForceX, FReal* sourceForceY, FReal* sourceForceZ, FReal* sourcePotential,
                                  const FReal targetX,const FReal targetY,const FReal targetZ, const FReal* targetPhysicalValue,
                                  FReal* targetForceX, FReal* targetForceY, FReal* targetForceZ, FReal* targetPotential
                                  ){
        // GradGradR potential
        FReal dx = sourceX - targetX;
        FReal dy = sourceY - targetY;
        FReal dz = sourceZ - targetZ;

        FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
        FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
        FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;

        FReal r[3]={dx,dy,dz};

        for(unsigned int i = 0 ; i < 3 ; ++i){
          FReal ri2=r[i]*r[i];
          for(unsigned int j = 0 ; j < 3 ; ++j){

            // potentials
            FReal potentialCoef;
            if(i==j)
              potentialCoef = inv_distance - ri2 * inv_distance_pow3;
            else
              potentialCoef = - r[i] * r[j] * inv_distance_pow3;

            // forces
            FReal rj2=r[j]*r[j];

            FReal coef[3]; 
            for(unsigned int k = 0 ; k < 3 ; ++k)
              coef[k]= -(targetPhysicalValue[j] * sourcePhysicalValue[j]);

            // Grad of RIJ kernel is RIJK kernel => use same expression as in FInterpMatrixKernel
            for(unsigned int k = 0 ; k < 3 ; ++k){
              if(i==j){
                if(j==k) //i=j=k
                  coef[k] *= FReal(3.) * ( FReal(-1.) + ri2 * inv_distance_pow2 ) * r[i] * inv_distance_pow3;
                else //i=j!=k
                  coef[k] *= ( FReal(-1.) + FReal(3.) * ri2 * inv_distance_pow2 ) * r[k] * inv_distance_pow3;
              }
              else{ //(i!=j)
                if(i==k) //i=k!=j
                  coef[k] *= ( FReal(-1.) + FReal(3.) * ri2 * inv_distance_pow2 ) * r[j] * inv_distance_pow3;
                else if(j==k) //i!=k=j
                  coef[k] *= ( FReal(-1.) + FReal(3.) * rj2 * inv_distance_pow2 ) * r[i] * inv_distance_pow3;
                else //i!=k!=j
                  coef[k] *= FReal(3.) * r[i] * r[j] * r[k] * inv_distance_pow2 * inv_distance_pow3;
              }
            }// k

            targetForceX[i] += coef[0];
            targetForceY[i] += coef[1];
            targetForceZ[i] += coef[2];
            targetPotential[i] += ( potentialCoef * sourcePhysicalValue[j] );

            sourceForceX[i] -= coef[0];
            sourceForceY[i] -= coef[1];
            sourceForceZ[i] -= coef[2];
            sourcePotential[i] += ( potentialCoef * targetPhysicalValue[j] );

          }// j
        }// i

    }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // PB: Test Tensorial Kernel ID_over_R
  // i.e. [[1 1 1]
  //       [1 1 1] * 1/R
  //       [1 1 1]]
  // Only use scalar phys val, potential and forces for now. TODO use vectorial ones.
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  /**
   * @brief FullMutualIOR
   */
  template <class ContainerClass>
  static void FullMutualIOR(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                            const int limiteNeighbors){

    const int nbParticlesTargets = inTargets->getNbParticles();
    const FReal*const targetsX = inTargets->getPositions()[0];
    const FReal*const targetsY = inTargets->getPositions()[1];
    const FReal*const targetsZ = inTargets->getPositions()[2];

    for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
      for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
        if( inNeighbors[idxNeighbors] ){
          const int nbParticlesSources = inNeighbors[idxNeighbors]->getNbParticles();
          const FReal*const sourcesX = inNeighbors[idxNeighbors]->getPositions()[0];
          const FReal*const sourcesY = inNeighbors[idxNeighbors]->getPositions()[1];
          const FReal*const sourcesZ = inNeighbors[idxNeighbors]->getPositions()[2];

          for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
            FReal dx = sourcesX[idxSource] - targetsX[idxTarget];
            FReal dy = sourcesY[idxSource] - targetsY[idxTarget];
            FReal dz = sourcesZ[idxSource] - targetsZ[idxTarget];

            FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
            FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
            FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;

            FReal r[3]={dx,dy,dz};

            for(unsigned int i = 0 ; i < 3 ; ++i){
              FReal*const targetsPotentials = inTargets->getPotentials(i);
              FReal*const targetsForcesX = inTargets->getForcesX(i);
              FReal*const targetsForcesY = inTargets->getForcesY(i);
              FReal*const targetsForcesZ = inTargets->getForcesZ(i);
              FReal*const sourcesPotentials = inNeighbors[idxNeighbors]->getPotentials(i);
              FReal*const sourcesForcesX = inNeighbors[idxNeighbors]->getForcesX(i);
              FReal*const sourcesForcesY = inNeighbors[idxNeighbors]->getForcesY(i);
              FReal*const sourcesForcesZ = inNeighbors[idxNeighbors]->getForcesZ(i);

              for(unsigned int j = 0 ; j < 3 ; ++j){
                const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues(j);
                const FReal*const sourcesPhysicalValues = inNeighbors[idxNeighbors]->getPhysicalValues(j);

                // potentials
                FReal potentialCoef;
                //if(i==j)
                potentialCoef = inv_distance;
                //else
                //  potentialCoef = FReal(0.);

                // forces
                FReal coef[3];
                for(unsigned int k = 0 ; k < 3 ; ++k){
                  //if(i==j){
                  coef[k] = + r[k] * inv_distance_pow3 * (targetsPhysicalValues[idxTarget] * sourcesPhysicalValues[idxSource]);
                  //}
                  //else{
                  // coef[k] = FReal(0.);
                  //}
                }// k

                targetsForcesX[idxTarget] += coef[0];
                targetsForcesY[idxTarget] += coef[1];
                targetsForcesZ[idxTarget] += coef[2];
                targetsPotentials[idxTarget] += ( potentialCoef * sourcesPhysicalValues[idxSource] );

                sourcesForcesX[idxSource] -= coef[0];
                sourcesForcesY[idxSource] -= coef[1];
                sourcesForcesZ[idxSource] -= coef[2];
                sourcesPotentials[idxSource] += potentialCoef * targetsPhysicalValues[idxTarget];

                             
              }// j
            }// i
          }
        }
      }
    }

    for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
      for(int idxSource = idxTarget + 1 ; idxSource < nbParticlesTargets ; ++idxSource){
        FReal dx = targetsX[idxSource] - targetsX[idxTarget];
        FReal dy = targetsY[idxSource] - targetsY[idxTarget];
        FReal dz = targetsZ[idxSource] - targetsZ[idxTarget];

        FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
        FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
        FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;

        FReal r[3]={dx,dy,dz};

        for(unsigned int i = 0 ; i < 3 ; ++i){
          FReal*const targetsPotentials = inTargets->getPotentials(i);
          FReal*const targetsForcesX = inTargets->getForcesX(i);
          FReal*const targetsForcesY = inTargets->getForcesY(i);
          FReal*const targetsForcesZ = inTargets->getForcesZ(i);

          for(unsigned int j = 0 ; j < 3 ; ++j){
            const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues(j);

            // potentials
            FReal potentialCoef;
            //if(i==j)
            potentialCoef = inv_distance;
            //else
            //  potentialCoef = FReal(0.);

            // forces
            FReal coef[3];
            for(unsigned int k = 0 ; k < 3 ; ++k){
              //if(i==j){
              coef[k] = + r[k] * inv_distance_pow3 * (targetsPhysicalValues[idxTarget] * targetsPhysicalValues[idxSource]);
              //}
              //else{
              //  coef[k] = FReal(0.);
              //}
            }// k

            targetsForcesX[idxTarget] += coef[0];
            targetsForcesY[idxTarget] += coef[1];
            targetsForcesZ[idxTarget] += coef[2];
            targetsPotentials[idxTarget] += ( potentialCoef * targetsPhysicalValues[idxSource] );

            targetsForcesX[idxSource] -= coef[0];
            targetsForcesY[idxSource] -= coef[1];
            targetsForcesZ[idxSource] -= coef[2];
            targetsPotentials[idxSource] += potentialCoef * targetsPhysicalValues[idxTarget];
                             
          }// j
        }// i

      }
    }
  }

  /**
   * @brief FullRemoteIOR
   */
  template <class ContainerClass>
  static void FullRemoteIOR(ContainerClass* const FRestrict inTargets, ContainerClass* const inNeighbors[],
                            const int limiteNeighbors){

    const int nbParticlesTargets = inTargets->getNbParticles();
    const FReal*const targetsX = inTargets->getPositions()[0];
    const FReal*const targetsY = inTargets->getPositions()[1];
    const FReal*const targetsZ = inTargets->getPositions()[2];

    for(int idxNeighbors = 0 ; idxNeighbors < limiteNeighbors ; ++idxNeighbors){
      for(int idxTarget = 0 ; idxTarget < nbParticlesTargets ; ++idxTarget){
        if( inNeighbors[idxNeighbors] ){
          const int nbParticlesSources = inNeighbors[idxNeighbors]->getNbParticles();
          const FReal*const sourcesX = inNeighbors[idxNeighbors]->getPositions()[0];
          const FReal*const sourcesY = inNeighbors[idxNeighbors]->getPositions()[1];
          const FReal*const sourcesZ = inNeighbors[idxNeighbors]->getPositions()[2];

          for(int idxSource = 0 ; idxSource < nbParticlesSources ; ++idxSource){
            // potential
            FReal dx = sourcesX[idxSource] - targetsX[idxTarget];
            FReal dy = sourcesY[idxSource] - targetsY[idxTarget];
            FReal dz = sourcesZ[idxSource] - targetsZ[idxTarget];

            FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
            FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
            FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;

            FReal r[3]={dx,dy,dz};

            for(unsigned int i = 0 ; i < 3 ; ++i){
              FReal*const targetsPotentials = inTargets->getPotentials(i);
              FReal*const targetsForcesX = inTargets->getForcesX(i);
              FReal*const targetsForcesY = inTargets->getForcesY(i);
              FReal*const targetsForcesZ = inTargets->getForcesZ(i);

              for(unsigned int j = 0 ; j < 3 ; ++j){
                const FReal*const targetsPhysicalValues = inTargets->getPhysicalValues(j);
                const FReal*const sourcesPhysicalValues = inNeighbors[idxNeighbors]->getPhysicalValues(j);

                // potentials
                FReal potentialCoef;
                //if(i==j)
                potentialCoef = inv_distance ;
                //else
                //  potentialCoef = FReal(0.);

                // forces
                FReal coef[3];
                for(unsigned int k = 0 ; k < 3 ; ++k){
                  //if(i==j){
                  coef[k] = + r[k] * inv_distance_pow3 * (targetsPhysicalValues[idxTarget] * sourcesPhysicalValues[idxSource]);
                  //}
                  //else{
                  //  coef[k] = FReal(0.);
                  //}
                }// k

                targetsForcesX[idxTarget] += coef[0];
                targetsForcesY[idxTarget] += coef[1];
                targetsForcesZ[idxTarget] += coef[2];
                targetsPotentials[idxTarget] += ( potentialCoef * sourcesPhysicalValues[idxSource] );

              }// j
            }// i

          }
        }
      }
    }
  }


  /**
   * @brief MutualParticlesIOR
   * @param sourceX
   * @param sourceY
   * @param sourceZ
   * @param sourcePhysicalValue
   * @param sourceForceX
   * @param sourceForceY
   * @param sourceForceZ
   * @param sourcePotential
   * @param targetX
   * @param targetY
   * @param targetZ
   * @param targetPhysicalValue
   * @param targetForceX
   * @param targetForceY
   * @param targetForceZ
   * @param targetPotential
   */
  static void MutualParticlesIOR(const FReal sourceX,const FReal sourceY,const FReal sourceZ, const FReal* sourcePhysicalValue,
                                 FReal* sourceForceX, FReal* sourceForceY, FReal* sourceForceZ, FReal* sourcePotential,
                                 const FReal targetX,const FReal targetY,const FReal targetZ, const FReal* targetPhysicalValue,
                                 FReal* targetForceX, FReal* targetForceY, FReal* targetForceZ, FReal* targetPotential
                                 ){
    // GradGradR potential
    FReal dx = sourceX - targetX;
    FReal dy = sourceY - targetY;
    FReal dz = sourceZ - targetZ;

    FReal inv_distance_pow2 = FReal(1.0) / (dx*dx + dy*dy + dz*dz);
    FReal inv_distance = FMath::Sqrt(inv_distance_pow2);
    FReal inv_distance_pow3 = inv_distance_pow2 * inv_distance;

    FReal r[3]={dx,dy,dz};

    for(unsigned int i = 0 ; i < 3 ; ++i){
      for(unsigned int j = 0 ; j < 3 ; ++j){

        // potentials
        FReal potentialCoef;
        //if(i==j)
        potentialCoef = inv_distance;
        //else
        //  potentialCoef = FReal(0.);

        // forces
        FReal coef[3];
        for(unsigned int k = 0 ; k < 3 ; ++k){
          //if(i==j){
          coef[k] = + r[k] * inv_distance_pow3 * (targetPhysicalValue[j] * sourcePhysicalValue[j]);
          //}
          //else{
          //  coef[k] = FReal(0.);
          //}
        }// k

        targetForceX[i] += coef[0];
        targetForceY[i] += coef[1];
        targetForceZ[i] += coef[2];
        targetPotential[i] += ( potentialCoef * sourcePhysicalValue[j] );

        sourceForceX[i] -= coef[0];
        sourceForceY[i] -= coef[1];
        sourceForceZ[i] -= coef[2];
        sourcePotential[i] += ( potentialCoef * targetPhysicalValue[j] );

      }// j
    }// i
  }

};

#endif // FP2P_HPP
