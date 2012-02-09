// ===================================================================================
// Logiciel initial: ScalFmm Version 0.5
// Co-auteurs : Olivier Coulaud, Bérenger Bramas.
// Propriétaires : INRIA.
// Copyright © 2011-2012, diffusé sous les termes et conditions d’une licence propriétaire.
// Initial software: ScalFmm Version 0.5
// Co-authors: Olivier Coulaud, Bérenger Bramas.
// Owners: INRIA.
// Copyright © 2011-2012, spread under the terms and conditions of a proprietary license.
// ===================================================================================
#ifndef FFMBKERNELSBLOCKBLAS_HPP
#define FFMBKERNELSBLOCKBLAS_HPP


#include "../Utils/FGlobal.hpp"
#include "../Utils/FBlas.hpp"
#include "../Components/FAbstractKernels.hpp"

#include "../Containers/FTreeCoordinate.hpp"

#include "../Utils/F3DPosition.hpp"
#include "../Utils/FComplexe.hpp"
#include "../Utils/FMath.hpp"
#include "../Utils/FTrace.hpp"

#include "FFmbKernels.hpp"

#include <iostream>


/**
* @author Berenger Bramas (berenger.bramas@inria.fr)
* @class FFmbKernelsBlockBlas
* @brief
* Please read the license
*
* This code is coming from the fmb library (see documentation to know more about it).
* It is a copy'n paste file with a few modifications.
* To be able to make link between this file and the originals ones you can
* look to the commentary before function or attributes declarations.
*
* This class is abstract because the fmb's algorithm is able to compute
* forces / potential / both
* So this class is the trunk and defines code used for each 3 computations.
*
* Needs cell to extend {FExtendFmbCell}
*/
template< class ParticleClass, class CellClass, class ContainerClass>
class FFmbKernelsBlockBlas {
protected:
    // _GRAVITATIONAL_
    static const int FMB_Info_eps_soft_square = 1;

    // MUST BE FALSE IN BLAS
    static const int FMB_Info_up_to_P_in_M2L = true;

    // Can be FMB_Info_P if user ask to -- if FMB_Info.up_to_P_in_M2L it true
    static const int FMB_Info_M2L_P = FMB_Info_up_to_P_in_M2L? FMB_Info_P : 2 * FMB_Info_P;
    static const int FMB_Info_M2L_exp_size = int(((FMB_Info_M2L_P)+1) * ((FMB_Info_M2L_P)+2) * 0.5);

    // Default value set in main
    static const int FMB_Info_ws = 1;

    // INTERACTION_LIST_SIZE_ALONG_1_DIM
    static const int size1Dim =  (2*(2*(FMB_Info_ws)+1) +1);
    // HALF_INTERACTION_LIST_SIZE_ALONG_1_DIM
    static const int halphSize1Dim =  (2*(FMB_Info_ws)+1);

    // EXPANSION_SIZE(FMB_Info.P)
    static const int FMB_Info_exp_size = int(((FMB_Info_P)+1) * ((FMB_Info_P)+2) * 0.5);
    // NEXP_SIZE(FMB_Info.P)
    static const int FMB_Info_nexp_size = (FMB_Info_P + 1) * (FMB_Info_P + 1);


    // tree height
    const int TreeHeight;

    // Width of the box at the root level
    FReal treeWidthAtRoot;

    // transfer_M2M_container size is specific to blas
    FComplexe transitionM2M[MaxTreeHeight][8][FMB_Info_nexp_size];
    // transfer_L2L_container
    FComplexe transitionL2L[MaxTreeHeight][8][FMB_Info_nexp_size];

    struct block_matrix_t {
        /* 'P', 'M' and 'N' values for this block: */
        int P_block;
        int M_block;
        int N_block;

        /* Block matrix values:
      * (stored as a one-dimensional array) */
        FComplexe *data;

        /* Info about bssm (biggest square sub-matrix,
       * 'square' in the number of blocks, in each dimension,
       * it contains): */
        int bssm_row_dim;
        int bssm_column_dim;


        /* Sub blocks: */
        struct block_matrix_t *below;
        struct block_matrix_t *right;

    };

    // transfer_container transfer_M2L_matrix - ff_block_matrix_Allocate_rec
    block_matrix_t* transferM2L[MaxTreeHeight][size1Dim][size1Dim][size1Dim];

    //[OK] spherical_harmonic_Outer_coefficients_array
    FReal sphereHarmoOuterCoef[FMB_Info_M2L_P+1];
    //[OK] spherical_harmonic_Inner_coefficients_array
    FReal sphereHarmoInnerCoef[FMB_Info_M2L_exp_size];

    FComplexe current_thread_Y[FMB_Info_exp_size];

    // p_Y_theta_derivated
    FComplexe current_thread_Y_theta_derivated[FMB_Info_exp_size];

    // pow_of_I_array
    static const FReal PiArrayInner[4];
    // pow_of_O_array
    static const FReal PiArrayOuter[4];

    // To store spherical position
    struct Spherical {
        FReal r, cosTheta, sinTheta, phi;
    };

    int expansion_Redirection_array_for_j[FMB_Info_M2L_P + 1 ];

    static const int FMB_Info_stop_for_block = 1;

    static const int FF_MATRIX_ROW_DIM = FMB_Info_exp_size;
    static const int FF_MATRIX_COLUMN_DIM = FMB_Info_nexp_size;
    static const int FF_MATRIX_SIZE = int(FF_MATRIX_ROW_DIM) * int(FF_MATRIX_COLUMN_DIM);

    //////////////////////////////////////////////////////////////////
    // Allocation
    //////////////////////////////////////////////////////////////////

    void expansion_Redirection_array_for_j_Initialize() {
        for( int h = 0; h <= FMB_Info_M2L_P ; ++h ){
            expansion_Redirection_array_for_j[h] = static_cast<int>( h * ( h + 1 ) * 0.5 );
        }
    }

    //spherical_harmonic_Outer_and_Inner_coefficients_array_Initialize
    void sphericalHarmonicInitialize(){
        // Outer coefficients:
        //std::cout << "sphereHarmoOuterCoef\n";
        FReal factOuter = 1.0;
        // in FMB code stoped at <= FMB_Info_M2L_P but this is not sufficient
        for(int idxP = 0 ; idxP <= FMB_Info_M2L_P; factOuter *= FReal(++idxP) ){
            this->sphereHarmoOuterCoef[idxP] = factOuter;
            //printf("spherical_harmonic_Outer_coefficients_array %.15e\n",this->sphereHarmoOuterCoef[idxP]);
            //printf("fact_l %.15e\n",factOuter);
            //printf("l %d\n",idxP);
        }

        // Inner coefficients:
        FReal* currentInner = this->sphereHarmoInnerCoef;
        FReal factInner = 1.0;
        FReal powN1idxP = 1.0;
        //std::cout << "sphereHarmoInnerCoef\n";
        for(int idxP = 0 ; idxP <= this->FMB_Info_M2L_P ; factInner *= FReal(++idxP), powN1idxP = -powN1idxP){
            for(int idxMP = 0, fact_l_m = int(factInner); idxMP <= idxP ; fact_l_m *= idxP+(++idxMP), ++currentInner){
                *currentInner = powN1idxP / FReal(fact_l_m);
                //std::cout << (*currentInner) << "\n";
            }
        }

        //for(int temp = 0 ; temp < 6 ; ++temp){
        //    std::cout << this->sphereHarmoInnerCoef[temp] << "\n";
        //}
    }


    block_matrix_t *ff_block_matrix_Allocate_rec(int M, int N, int P){
        block_matrix_t *res = new block_matrix_t();
        int stop_for_block = FMB_Info_stop_for_block;

        /* Initialize fields of '*res': */
        res->P_block = P;
        res->M_block = M;
        res->N_block = N;
        res->data = NULL;
        res->bssm_row_dim = 0;
        res->bssm_column_dim = 0;
        res->below = NULL;
        res->right = NULL;


        if (P <= stop_for_block){
            /*************** Treat terminal case: ***************/
            /* The 'P+1' bands are stored one after the other, starting from res->data,
         * so that the leading dimension is the smallest one. */
            int PP = P+1;
            int MM = M+1;
            int total =  (int) (PP+1) * PP * (PP*PP + 4*MM*PP + 4*PP*N - PP - 4*N +2*MM + 12*MM*N) / 12;

            res->data = new FComplexe[total];
        }
        else {
            /*************** Recursive case: ***************/
            int q = P/2;
            int q_rec = ((P%2 == 0) ? q-1 : q);

            /* Matrix-vector product with the biggest square sub-matrix ('square' in the number of
         * blocks, in each dimension, it contains). */
            /* bssm: biggest square sub-matrix */

            res->bssm_row_dim =  (int) ( (q+1) * (q*0.5 + M +1) ); /* sum_{k=0}^{q} (M+k+1) */
            res->bssm_column_dim = (int) ( (q+1) * (2*N + q + 1) ); /* sum_{k=0}^{q} (2*(N+k)+1) */

            res->data = new FComplexe[res->bssm_row_dim* res->bssm_column_dim];

            res->below = ff_block_matrix_Allocate_rec(M + q+1, N, q_rec);
            res->right  = ff_block_matrix_Allocate_rec(M, N + q+1, q_rec);
        }

        return res;
    }

    // transfer_L2L_Allocate
    // transfer_M2M_Allocate
    // transfer_M2L_Allocate
    void transferAllocate(){
        // M2M L2L
        /*this->transitionM2M = new FComplexe**[TreeHeight];
        this->transitionL2L = new FComplexe**[TreeHeight];

        for(int idxLevel = 0 ; idxLevel < TreeHeight ; ++idxLevel){

            this->transitionM2M[idxLevel] = new FComplexe*[8];
            this->transitionL2L[idxLevel] = new FComplexe*[8];

            for(int idxChild = 0; idxChild < 8; ++idxChild){
                this->transitionM2M[idxLevel][idxChild] = new FComplexe[FMB_Info_exp_size];
                this->transitionL2L[idxLevel][idxChild] = new FComplexe[FMB_Info_exp_size];
            }
        }*/
        // M2L
        //this->transferM2L = new FComplexe****[TreeHeight+1];

        for(int idxLevel = 0; idxLevel < TreeHeight; ++idxLevel){
            //this->transferM2L[idxLevel] = new FComplexe***[this->size1Dim];

            for(int idxD1 = 0 ; idxD1 < this->size1Dim; ++idxD1){
                //this->transferM2L[idxLevel][idxD1] = new FComplexe**[this->size1Dim];

                for(int idxD2 = 0; idxD2 < this->size1Dim; ++idxD2){
                    //this->transferM2L[idxLevel][idxD1][idxD2] = new FComplexe*[this->size1Dim];

                    for(int idxD3 = 0; idxD3 < this->size1Dim; ++idxD3){
                        const int x = idxD1 - this->halphSize1Dim;
                        const int y = idxD2 - this->halphSize1Dim;
                        const int z = idxD3 - this->halphSize1Dim;

                        if( ( x*x + y*y + z*z ) >= ( 3*this->FMB_Info_ws*this->FMB_Info_ws + 0.1 ) ){
                            this->transferM2L[idxLevel][idxD1][idxD2][idxD3] = ff_block_matrix_Allocate_rec(0,0,FMB_Info_P);
                        }
                        else {
                            this->transferM2L[idxLevel][idxD1][idxD2][idxD3] = NULL;
                        }
                    }
                }
            }
        }
    }

    void ff_block_matrix_Free(block_matrix_t *p){
        if (p != NULL){
            /* Recursive calls: */
            ff_block_matrix_Free(p->below);
            ff_block_matrix_Free(p->right);

            /* Free current block matrix: */
            delete [] (p->data);

            /* Free 'p' itself: */
            delete(p);
        }
    }

    // transfer_L2L_free
    // transfer_M2M_free
    // transfer_M2L_free
    void transferDeallocate(){
        // M2M L2L
        /*for(int idxLevel = 0 ; idxLevel < TreeHeight ; ++idxLevel){
            for(int idxChild = 0; idxChild < 8; ++idxChild){
                delete [] this->transitionM2M[idxLevel][idxChild];
                delete [] this->transitionL2L[idxLevel][idxChild];
            }
            delete [] this->transitionM2M[idxLevel];
            delete [] transitionL2L[idxLevel];
        }
        delete [] this->transitionM2M;
        delete [] this->transitionL2L;*/
        // M2L
        for(int idxLevel = 0 ; idxLevel < TreeHeight; ++idxLevel){
            for(int idxD1 = 0 ; idxD1 < this->size1Dim ; ++idxD1){
                for(int idxD2 = 0 ; idxD2 < this->size1Dim ; ++idxD2){
                    for(int idxD3 = 0 ; idxD3 < this->size1Dim; ++idxD3){
                        ff_block_matrix_Free(this->transferM2L[idxLevel][idxD1][idxD2][idxD3]);
                    }
                    //delete [] this->transferM2L[idxLevel][idxD1][idxD2];
                }
                //delete [] this->transferM2L[idxLevel][idxD1];
            }
            //delete [] this->transferM2L[idxLevel];
        }
        //delete [] this->transferM2L;
    }

    //////////////////////////////////////////////////////////////////
    // Utils
    //////////////////////////////////////////////////////////////////

    // position_2_r_cos_th_sin_th_ph
    Spherical positionTsmphere(const F3DPosition& inVector){
        const FReal x2y2 = (inVector.getX() * inVector.getX()) + (inVector.getY() * inVector.getY());

        Spherical outSphere;

        outSphere.r = FMath::Sqrt( x2y2 + (inVector.getZ() * inVector.getZ()));
        outSphere.phi = FMath::Atan2(inVector.getY(),inVector.getX());
        outSphere.cosTheta = inVector.getZ() / outSphere.r; // cos_th = z/r
        outSphere.sinTheta = FMath::Sqrt(x2y2) / outSphere.r; // sin_th = sqrt(x^2 + y^2)/r

        return outSphere;
    }

    // associated_Legendre_function_Fill_complete_array_of_values_for_cos
    void legendreFunction( const int lmax, const FReal inCosTheta, const FReal inSinTheta, FReal* const outResults ){
        // l=0:         results[current++] = 1.0; // P_0^0(cosTheta) = 1
        int idxCurrent = 0;
        outResults[idxCurrent++] = 1.0;

        // l=1:
        // Compute P_1^{0} using (3) and store it into results_array:
        outResults[idxCurrent++] = inCosTheta;

        // Compute P_1^1 using (2 bis) and store it into results_array
        const FReal invSinTheta = -inSinTheta; // somx2 => -sinTheta
        outResults[idxCurrent++] = invSinTheta;

        // l>1:
        int idxCurrent1m = 1; //pointer on P_{l-1}^m P_1^0
        int idxCurrent2m = 0; //pointer on P_{l-2}^m P_0^0
        FReal fact = 3.0;

        // Remark: p_results_array_l_minus_1_m and p_results_array_l_minus_2_m
        // just need to be incremented at each iteration.
        for(int idxl = 2; idxl <= lmax ; ++idxl ){
            for( int idxm = 0; idxm <= idxl - 2 ; ++idxm , ++idxCurrent , ++idxCurrent1m , ++idxCurrent2m ){
                // Compute P_l^m, l >= m+2, using (1) and store it into results_array:
                outResults[idxCurrent] = (inCosTheta * FReal( 2 * idxl - 1 ) * outResults[idxCurrent1m] - FReal( idxl + idxm - 1 )
                                          * outResults[idxCurrent2m] ) / FReal( idxl - idxm );
                /*printf("\tres (%.18e) = inCosTheta (%.18e) idxl (%d) outResults 1m (%.18e) idxm (%d) outResults 2m (%.18e)\n",
                       outResults[idxCurrent],inCosTheta,idxl,outResults[idxCurrent1m],idxm,outResults[idxCurrent2m]);
                printf("\t\t temp1 = %.18e temp2 = %.18e temp3 = %.18e\n",
                       inCosTheta * ( 2 * idxl - 1 ) * outResults[idxCurrent1m],
                       ( idxl + idxm - 1 ) * outResults[idxCurrent2m],
                       (inCosTheta * ( 2 * idxl - 1 ) * outResults[idxCurrent1m] - ( idxl + idxm - 1 ) * outResults[idxCurrent2m] ));*/
            }
            // p_results_array_l_minus_1_m now points on P_{l-1}^{l-1}

            // Compute P_l^{l-1} using (3) and store it into ptrResults:
            outResults[idxCurrent++] = inCosTheta * FReal( 2 * idxl - 1 ) * outResults[idxCurrent1m];

            // Compute P_l^l using (2 bis) and store it into results_array:
            outResults[idxCurrent++] = fact * invSinTheta * outResults[idxCurrent1m];

            fact += FReal(2.0);
            ++idxCurrent1m;
        }

        //for(int idxprint = 0 ; idxprint <= FMB_Info_M2L_P ; ++idxprint){
        //    printf("\t legendre[%d] = %.15e\n",idxprint ,outResults[idxprint]);
        //}
    }

    // spherical_harmonic_Inner
    //2.7 these
    void harmonicInner(const Spherical& inSphere, FComplexe* const outResults){
        // p_precomputed_cos_and_sin_array
        FComplexe cosSin[FMB_Info_M2L_P + 1];

        for(int idxl = 0 , idxlMod4 = 0; idxl <= FMB_Info_P ; ++idxl, ++idxlMod4){
            if(idxlMod4 == 4) idxlMod4 = 0;
            const FReal angleinter = FReal(idxl) * inSphere.phi;
            const FReal angle = angleinter + this->PiArrayInner[idxlMod4];

            cosSin[idxl].setReal( FMath::Sin(angle + FMath::FPiDiv2) );
            cosSin[idxl].setImag( FMath::Sin(angle) );

            /*printf("%d=%.15e/%.15e (angle %.15e=%d * %.15e + %.15e // (%.15e))\n",
                   idxl,cosSin[idxl].getReal(),cosSin[idxl].getImag(),
                   angle,idxl,inSphere.phi,this->PiArrayInner[idxlMod4],angleinter);*/

            /*printf("sin(%.15e)  = %.15e\n",
                   angle + FMath::FPiDiv2,FMath::Sin(angle + FMath::FPiDiv2));*/
        }

        // p_associated_Legendre_function_Array
        FReal legendre[FMB_Info_M2L_exp_size];
        legendreFunction(FMB_Info_P,inSphere.cosTheta, inSphere.sinTheta, legendre);
        /*printf("FMB_Info_M2L_exp_size=%d\n",FMB_Info_M2L_exp_size);
        for(int temp = 0 ; temp < FMB_Info_M2L_exp_size ; ++temp){
            printf("%.15e\n",legendre[temp]);
        }*/

        FComplexe* currentResult = outResults;
        int idxLegendre = 0;//ptr_associated_Legendre_function_Array
        int idxSphereHarmoCoef = 0;
        FReal idxRl = 1.0 ;

        //printf("lmax = %d\n",FMB_Info_P);
        for(int idxl = 0; idxl <= FMB_Info_P ; ++idxl, idxRl *= inSphere.r){
            for(int idxm = 0 ; idxm <= idxl ; ++idxm, ++currentResult, ++idxSphereHarmoCoef, ++idxLegendre){
                const FReal magnitude = this->sphereHarmoInnerCoef[idxSphereHarmoCoef] * idxRl * legendre[idxLegendre];
                currentResult->setReal( magnitude * cosSin[idxm].getReal() );
                currentResult->setImag( magnitude * cosSin[idxm].getImag() );

                /*printf("\t\tl = %d m = %d\n",idxl,idxm);
                printf("\t\tmagnitude (%.15e)  = idxRl (%.15e) * sphereHarmoInnerCoef (%.15e) * legendre (%.15e)\n",
                       magnitude,idxRl,this->sphereHarmoInnerCoef[idxSphereHarmoCoef],legendre[idxLegendre]);
                printf("\t\tresult real=%.15e imag=%.15e\n",
                       currentResult->getReal(),currentResult->getImag());*/
            }
        }

    }
    // spherical_harmonic_Outer
    void harmonicOuter(const Spherical& inSphere, FComplexe* const outResults){
        // p_precomputed_cos_and_sin_array
        FComplexe cosSin[FMB_Info_M2L_P + 1];

        for(int idxl = 0, idxlMod4 = 0; idxl <= FMB_Info_M2L_P ; ++idxl, ++idxlMod4){
            if(idxlMod4 == 4) idxlMod4 = 0;
            const FReal angle = FReal(idxl) * inSphere.phi + this->PiArrayOuter[idxlMod4];

            cosSin[idxl].setReal( FMath::Sin(angle + FMath::FPiDiv2) );
            cosSin[idxl].setImag( FMath::Sin(angle) );

            //printf("l=%d \t inSphere.phi=%.15e \t this->PiArray[idxlMod4]=%.15e \t angle=%.15e \t FMath::Sin(angle + FMath::FPiDiv2)=%.15e \t FMath::Sin(angle)=%.15e\n",
            //        idxl, inSphere.phi, this->PiArrayOuter[idxlMod4], angle, FMath::Sin(angle + FMath::FPiDiv2) , FMath::Sin(angle));
        }

        // p_associated_Legendre_function_Array
        FReal legendre[FMB_Info_M2L_exp_size];
        legendreFunction(FMB_Info_M2L_P,inSphere.cosTheta, inSphere.sinTheta, legendre);

        int idxLegendre = 0;
        FComplexe* currentResult = outResults;

        const FReal invR = 1/inSphere.r;
        FReal idxRl1 = invR;
        for(int idxl = 0 ; idxl <= FMB_Info_M2L_P ; ++idxl, idxRl1 *= invR){
            for(int idxm = 0 ; idxm <= idxl ; ++idxm, ++currentResult, ++idxLegendre){
                const FReal magnitude = this->sphereHarmoOuterCoef[idxl-idxm] * idxRl1 * legendre[idxLegendre];
                currentResult->setReal( magnitude * cosSin[idxm].getReal() );
                currentResult->setImag( magnitude * cosSin[idxm].getImag() );
                //printf("l=%d\t m=%d\t idxRl1=%.15e\t magnitude=%.15e\n",idxl,idxm,idxRl1,magnitude);
                //printf("l=%d\t m=%d\t cosSin[idxm].getReal()=%.15e\t cosSin[idxm].getImag()=%.15e\n",
                //       idxl,idxm,cosSin[idxm].getReal(),cosSin[idxm].getImag());
                //printf("this->sphereHarmoOuterCoef[idxl-idxm] = %.15e \t this->legendre[idxLegendre] = %.15e \n",
                //       this->sphereHarmoOuterCoef[idxl-idxm],
                //       this->legendre[idxLegendre]);

                //for(int idxTemp = 0 ; idxTemp <= idxl-idxm ; idxTemp++ ){
                //    printf("\t this->sphereHarmoOuterCoef[%d] = %.15e\n", idxTemp,this->sphereHarmoOuterCoef[idxTemp] );
                //}
            }
        }
    }

    /** spherical_harmonic_Inner_and_theta_derivated
        * Returns the value of the partial derivative of the spherical harmonic
        *relative to theta. We have for all m such that -(l-1) <= m <= l-1 :
        *
        *(d H_l^m(theta, phi))/(d theta)
        *= (-1)^m sqrt((l-|m|)!/(l+|m|)!) (d P_l^{|m|}(cos theta))/(d theta) e^{i.m.phi}
        *= (-1)^m sqrt((l-|m|)!/(l+|m|)!) 1/sqrt{1-cos^2 theta} [l cos theta P_l^{|m|}(cos theta) - (l+|m|) P_{l-1}^{|m|}(cos theta) ] e^{i.m.phi}
        *Since theta is in the range [0, Pi], we have: sin theta > 0 and therefore
        *sqrt{1-cos^2 theta} = sin theta. Thus:
        *
        *(d H_l^m(theta, phi))/(d theta)
        *= (-1)^m sqrt((l-|m|)!/(l+|m|)!) 1/(sin theta) [l cos theta P_l^{|m|}(cos theta) - (l+|m|) P_{l-1}^{|m|}(cos theta) ] e^{i.m.phi}
        *For |m|=l, we have~:
        *(d H_l^l(theta, phi))/(d theta)
        *= (-1)^m sqrt(1/(2l)!) 1/(sin theta) [l cos theta P_l^l(cos theta) ] e^{i.m.phi}
        *
        *Remark: for 0<m<=l:
        *(d H_l^{-m}(theta, phi))/(d theta) = [(d H_l^{-m}(theta, phi))/(d theta)]*
        *
        *
        *
        *Therefore, we have for (d Inner_l^m(r, theta, phi))/(d theta):
        *
        *|m|<l: (d Inner_l^m(r, theta, phi))/(d theta) =
        *(i^m (-1)^l / (l+|m|)!) 1/(sin theta) [l cos theta P_l^{|m|}(cos theta) - (l+|m|) P_{l-1}^{|m|}(cos theta) ] e^{i.m.phi} r^l
        *|m|=l: (d Inner_l^m(r, theta, phi))/(d theta) =
        *(i^m (-1)^l / (l+|m|)!) 1/(sin theta) [l cos theta P_l^l(cos theta) ] e^{i.m.phi} r^l
        *
        *
      */
    void harmonicInnerThetaDerivated(
            const Spherical& inSphere,
            FComplexe * results_array,
            FComplexe * theta_derivated_results_array
            ){

        //printf("HarmoInnerTheta \t lmax = %d \t r = %.15e \t cos_theta = %.15e \t sin_theta = %.15e \t phi = %.15e\n",
        //       FMB_Info_P,inSphere.r,inSphere.cosTheta,inSphere.sinTheta,inSphere.phi);

        // p_precomputed_cos_and_sin_array
        FComplexe cosSin[FMB_Info_M2L_P + 1];

        // Initialization of precomputed_cos_and_sin_array:
        for(int idxm = 0 , idxmMod4 = 0; idxm <= FMB_Info_P ; ++idxm, ++idxmMod4){
            if(idxmMod4 == 4) idxmMod4 = 0;
            const FReal angle = FReal(idxm) *inSphere.phi + PiArrayInner[idxmMod4];
            cosSin[idxm].setReal(FMath::Sin(angle + FMath::FPiDiv2));
            cosSin[idxm].setImag(FMath::Sin(angle));

            //printf("l=%d \t inSphere.phi=%.15e \t this->PiArrayOuter[idxlMod4]=%.15e \t angle=%.15e \t FMath::Sin(angle + FMath::FPiDiv2)=%.15e \t FMath::Sin(angle)=%.15e\n",
            //        idxm, inSphere.phi, this->PiArrayInner[idxmMod4], angle, FMath::Sin(angle + FMath::FPiDiv2) , FMath::Sin(angle));
        }

        // p_associated_Legendre_function_Array
        FReal legendre[FMB_Info_M2L_exp_size];
        // Initialization of associated_Legendre_function_Array:
        legendreFunction(FMB_Info_P, inSphere.cosTheta, inSphere.sinTheta, legendre);


        FComplexe *p_term = results_array;
        FComplexe *p_theta_derivated_term = theta_derivated_results_array;
        FReal *p_spherical_harmonic_Inner_coefficients_array = sphereHarmoInnerCoef;
        FReal *ptr_associated_Legendre_function_Array = legendre;
        FReal *start_ptr_associated_Legendre_function_Array = ptr_associated_Legendre_function_Array;

        // r^l
        FReal r_l = 1.0;
        for (int l = 0 ; l <= FMB_Info_P ; ++l, r_l *= inSphere.r){
            FReal magnitude;
            // m<l:
            int m = 0;
            for(; m < l ; ++m, ++p_term, ++p_theta_derivated_term, ++p_spherical_harmonic_Inner_coefficients_array, ++ptr_associated_Legendre_function_Array){
                magnitude = (*p_spherical_harmonic_Inner_coefficients_array) * r_l * (*ptr_associated_Legendre_function_Array);

                // Computation of Inner_l^m(r, theta, phi):
                p_term->setReal( magnitude * cosSin[m].getReal());
                p_term->setImag( magnitude * cosSin[m].getImag());

                /*printf("%d/%d - magnitude=%.15e ptr_precomputed_cos_and_sin_array real=%.15e imag=%.15e p_term real=%.15e imag=%.15e\n",
                       l,m,
                       magnitude,
                       cosSin[m].getReal(),
                       cosSin[m].getImag(),
                       p_term->getReal(),
                       p_term->getImag());*/
                /*printf("\t p_spherical_harmonic_Inner_coefficients_array = %.15e \t ptr_associated_Legendre_function_Array = %.15e \t r_l = %.15e\n",
                       *p_spherical_harmonic_Inner_coefficients_array,
                       *ptr_associated_Legendre_function_Array,
                       r_l
                       );*/

                // Computation of {\partial Inner_l^m(r, theta, phi)}/{\partial theta}:
                magnitude = (*p_spherical_harmonic_Inner_coefficients_array) * FReal(r_l) * ((FReal(l)*inSphere.cosTheta*(*ptr_associated_Legendre_function_Array)
                                                                                       - FReal(l+m)*(*(start_ptr_associated_Legendre_function_Array + expansion_Redirection_array_for_j[l-1] + m))) / inSphere.sinTheta);
                p_theta_derivated_term->setReal(magnitude * cosSin[m].getReal());
                p_theta_derivated_term->setImag(magnitude * cosSin[m].getImag());

                //printf("magnitude=%.15e r_l=%.15e p_spherical_harmonic_Inner_coefficients_array=%.15e real=%.15e imag=%.15e\n",
                //       magnitude,r_l,*p_spherical_harmonic_Inner_coefficients_array,p_theta_derivated_term->getReal(),p_theta_derivated_term->getImag());
            }

            // m=l:
            // Computation of Inner_m^m(r, theta, phi):
            magnitude = (*p_spherical_harmonic_Inner_coefficients_array) * r_l * (*ptr_associated_Legendre_function_Array);
            p_term->setReal(magnitude * cosSin[m].getReal());
            p_term->setImag(magnitude * cosSin[m].getImag());

            /*printf("%d - magnitude=%.15e ptr_precomputed_cos_and_sin_array real=%.15e imag=%.15e p_term real=%.15e imag=%.15e\n",
                   l,
                   magnitude,
                   cosSin[m].getReal(),
                   cosSin[m].getImag(),
                   p_term->getReal(),
                   p_term->getImag());*/
            /*printf("\t p_spherical_harmonic_Inner_coefficients_array = %.15e \t ptr_associated_Legendre_function_Array = %.15e \t r_l = %.15e\n",
                   *p_spherical_harmonic_Inner_coefficients_array,
                   *ptr_associated_Legendre_function_Array,
                   r_l
                   );*/

            // Computation of {\partial Inner_m^m(r, theta, phi)}/{\partial theta}:
            magnitude = (*p_spherical_harmonic_Inner_coefficients_array) * FReal(r_l) * (FReal(m) * inSphere.cosTheta * (*ptr_associated_Legendre_function_Array) / inSphere.sinTheta);
            p_theta_derivated_term->setReal(magnitude * cosSin[m].getReal());
            p_theta_derivated_term->setImag(magnitude * cosSin[m].getImag());

            //printf("magnitude=%.15e r_l=%.15e p_spherical_harmonic_Inner_coefficients_array=%.15e real=%.15e imag=%.15e\n",
            //       magnitude,r_l,*p_spherical_harmonic_Inner_coefficients_array,p_theta_derivated_term->getReal(),p_theta_derivated_term->getImag());

            ++p_term;
            ++p_theta_derivated_term;
            ++p_spherical_harmonic_Inner_coefficients_array;
            ++ptr_associated_Legendre_function_Array;
        }
    }

    //////////////////////////////////////////////////////////////////
    // Precompute
    //////////////////////////////////////////////////////////////////

    // transfer_M2M_Precompute_all_levels
    // transfer_L2L_Precompute_all_levels
    void precomputeM2M(){
        FReal treeWidthAtLevel = this->treeWidthAtRoot/2;

        for(int idxLevel = 0 ; idxLevel < TreeHeight - 1 ; ++idxLevel ){
            const F3DPosition father(treeWidthAtLevel,treeWidthAtLevel,treeWidthAtLevel);
            treeWidthAtLevel /= 2;

            //std::cout << "[precomputeM2M]treeWidthAtLevel=" << treeWidthAtLevel << "\n";
            //printf("\tidxLevel=%d\tFather.x=%.15e\tFather.y=%.15e\tFather.z=%.15e\n",idxLevel,father.getX(),father.getY(),father.getZ());

            for(int idxChild = 0 ; idxChild < 8 ; ++idxChild ){
                FTreeCoordinate childBox;
                childBox.setPositionFromMorton(idxChild,1);

                const F3DPosition M2MVector (
                        father.getX() - (treeWidthAtLevel * FReal(1 + (childBox.getX() * 2))),
                        father.getY() - (treeWidthAtLevel * FReal(1 + (childBox.getY() * 2))),
                        father.getZ() - (treeWidthAtLevel * FReal(1 + (childBox.getZ() * 2)))
                        );

                harmonicInner(positionTsmphere(M2MVector),this->transitionM2M[idxLevel][idxChild]);

                /*printf("[M2M_vector]%d/%d = %.15e/%.15e/%.15e\n", idxLevel , idxChild , M2MVector.getX() , M2MVector.getY() , M2MVector.getZ() );
                Spherical sphericalM2M = positionTsmphere(M2MVector);
                printf("[M2M_vectorSpherical]%d/%d = %.15e/%.15e/%.15e/%.15e\n",
                       idxLevel , idxChild , sphericalM2M.r , sphericalM2M.cosTheta , sphericalM2M.sinTheta , sphericalM2M.phi );
                for(int idxExpSize = 0 ; idxExpSize < FMB_Info_exp_size ; ++idxExpSize){
                    printf("transitionM2M[%d][%d][%d]=%.15e/%.15e\n", idxLevel , idxChild , idxExpSize , this->transitionM2M[idxLevel][idxChild][idxExpSize].getReal(),this->transitionM2M[idxLevel][idxChild][idxExpSize].getImag());
                }*/

                const F3DPosition L2LVector (
                        (treeWidthAtLevel * FReal(1 + (childBox.getX() * 2))) - father.getX(),
                        (treeWidthAtLevel * FReal(1 + (childBox.getY() * 2))) - father.getY(),
                        (treeWidthAtLevel * FReal(1 + (childBox.getZ() * 2))) - father.getZ()
                        );

                harmonicInner(positionTsmphere(L2LVector),this->transitionL2L[idxLevel][idxChild]);

            }
        }
    }

    void ff_block_matrix_Fill_data(FComplexe* transfer_exp,
                                   FComplexe* data,
                                   int M,
                                   int N,
                                   int nb_block_rows /* number of rows (counted by block) */,
                                   int nb_block_columns /* number of colums (counted by block) */){
        int NN, MM;
        int n, m;
        int k;
        int P = FMB_Info_P;

        /* matrix stored by rows */
        int NN_stop;

        for (MM=M; MM <= M + nb_block_rows - 1 ; ++MM){
            for (m=0;
                 m<=MM; /* Only positive orders for local expansion: */
                 ++m){

                NN_stop = FMath::Min(N + nb_block_columns - 1, P-MM);

                for (NN=N; NN <= NN_stop; ++NN){
                    for (n=0;
                         n<=2*NN;
                         ++n,
                         ++data){

                        /* Not optimized: */
                        k = NN-n-m;
                        if (k < 0){
                            const int pow_of_minus_1 = ((k%2) ? -1 : 1);
                            data->setReal( FReal(pow_of_minus_1) * (transfer_exp + expansion_Redirection_array_for_j[MM+NN] - k)->getReal());
                            data->setImag( FReal(-pow_of_minus_1) * (transfer_exp + expansion_Redirection_array_for_j[MM+NN] - k)->getImag());
                        }
                        else {
                            *data = *(transfer_exp + expansion_Redirection_array_for_j[MM+NN] + k);
                        }

                        //printf("MM %d m %d NN %d ; data %.15e %.15e\n",
                        //       MM,m,NN,data->getReal(),data->getImag());
                    } /* for m */
                } /* for MM */

            } /* for n */
        } /* for NN */

    }

    void ff_matrix_Convert_exp_2_ff_block_matrix(FComplexe* const transfer_exp,
                                                 block_matrix_t *p_transfer_block_matrix){
        int P = p_transfer_block_matrix->P_block;
        int M = p_transfer_block_matrix->M_block;
        int N = p_transfer_block_matrix->N_block;


        /* "p_transfer_block_matrix->below == NULL" implies
       * "p_transfer_block_matrix->right == NULL" too.
       *
       * We can also test: " if (ff_block_matrix_Is_terminal_case(M, N, P, FMB_Info.stop_for_block)) "
       */
        if (p_transfer_block_matrix->below == NULL) {
            /*************** Treat terminal case: ***************/
            /* The 'P+1' bands are stored one after the other, starting from p_transfer_block_matrix->data,
         * so that the leading dimension is the smallest one. */
            int row_nb ;
            int column_nb ;
            int band_number;
            int P_bn_1; /* will contain ' P - band_number +1 ' */

            FComplexe* data = p_transfer_block_matrix->data;

            P_bn_1 = P+1;
            for (band_number=0; band_number<=P; ++band_number){
                row_nb =  M + band_number + 1;
                column_nb = (P_bn_1) * (P_bn_1 + 2*N);

                ff_block_matrix_Fill_data(transfer_exp, data, M + band_number, N, 1, P_bn_1);
                data += row_nb * column_nb;  /* set data to start of next band */
                --P_bn_1;
            }
        }
        else {
            /*************** Recursive case: ***************/
            int q = P/2;

            /* Fill the biggest square sub-matrix ('square' in the number of
         * blocks, in each dimension, it contains). */
            /* bssm: biggest square sub-matrix */
            ff_block_matrix_Fill_data(transfer_exp, p_transfer_block_matrix->data, M, N, q+1, q+1);


            /* Recursive calls: */
            /* Sub-matrix "below": */
            ff_matrix_Convert_exp_2_ff_block_matrix(transfer_exp, p_transfer_block_matrix->below);

            /* Sub-matrix "on the right": */
            ff_matrix_Convert_exp_2_ff_block_matrix(transfer_exp, p_transfer_block_matrix->right);

        }
    }


    // transfer_M2L_Precompute_all_levels
    void precomputeM2L(){
        //[Blas] FComplexe tempComplexe[FMB_Info_M2L_exp_size];
        //printf("FMB_Info.M2L_exp_size = %d\n",FMB_Info_M2L_exp_size);

        FReal treeWidthAtLevel = this->treeWidthAtRoot;
        for(int idxLevel = 0 ; idxLevel < TreeHeight ; ++idxLevel ){
            //printf("level = %d \t width = %lf\n",idxLevel,treeWidthAtLevel);
            for( int idxd1 = 0; idxd1 < this->size1Dim ; ++idxd1 ){

                for( int idxd2 = 0; idxd2 < this->size1Dim ; ++idxd2 ){

                    for( int idxd3 = 0; idxd3 < this->size1Dim ; ++idxd3 ){
                        const FReal x = FReal(idxd1 - this->halphSize1Dim);
                        const FReal y = FReal(idxd2 - this->halphSize1Dim);
                        const FReal z = FReal(idxd3 - this->halphSize1Dim);

                        //printf("x=%ld \t y=%ld \t z=%ld\n",x,y,z);

                        if( ( x*x + y*y + z*z ) >= ( 3*FMB_Info_ws*FMB_Info_ws + 0.1 ) ){
                            const F3DPosition relativePos( x*treeWidthAtLevel , y*treeWidthAtLevel , z*treeWidthAtLevel );


                            //printf("blas\n");
                            //printf("transferM2L[%d][%d][%d][%d]\n", idxLevel, idxd1, idxd2, idxd3);
                            FComplexe tempComplexe[FMB_Info_M2L_exp_size];
                            harmonicOuter(positionTsmphere(relativePos),tempComplexe);

                            //ff_matrix_Convert_exp_2_transfer_M2L_matrix
                            ff_matrix_Convert_exp_2_ff_block_matrix(tempComplexe,this->transferM2L[idxLevel][idxd1][idxd2][idxd3]);

                            /*for(int idxTemp = 0 ; idxTemp < FMB_Info_M2L_exp_size ; ++idxTemp){
                                    printf("transferM2L[%d][%d][%d][%d][%d]=%.15e/%.15e\n",
                                           idxLevel,idxd1,idxd2,idxd3,idxTemp,
                                           tempComplexe[idxTemp].getReal(),
                                           tempComplexe[idxTemp].getImag());
                            }*/
                        }

                    }
                }
            }
            treeWidthAtLevel /= 2;
        }
    }

    void buildPrecompute(){
        expansion_Redirection_array_for_j_Initialize();
        sphericalHarmonicInitialize();
        transferAllocate();

        precomputeM2M();
        precomputeM2L();
    }

public:
    FFmbKernelsBlockBlas(const int inTreeHeight, const FReal inTreeWidth) :
            TreeHeight(inTreeHeight),treeWidthAtRoot(inTreeWidth) {
        buildPrecompute();
    }

    FFmbKernelsBlockBlas(const FFmbKernelsBlockBlas& other)
        : TreeHeight(other.TreeHeight), treeWidthAtRoot(other.treeWidthAtRoot) {
        buildPrecompute();
    }

    virtual void init(){

         
    }

    /** Default destructor */
    virtual ~FFmbKernelsBlockBlas(){
        transferDeallocate();
    }


    /////////////////////////////////////////////////////////////////////////////////
    //    Upward
    /////////////////////////////////////////////////////////////////////////////////

    /** OK!
    * expansion_P2M_add
    * Multipole expansion with m charges q_i in Q_i=(rho_i, alpha_i, beta_i)
    *whose relative coordinates according to *p_center are:
    *Q_i - *p_center = (rho'_i, alpha'_i, beta'_i);
    *
    *For j=0..P, k=-j..j, we have:
    *
    *M_j^k = (-1)^j { sum{i=1..m} q_i Inner_j^k(rho'_i, alpha'_i, beta'_i) }
    *
    *However the extern loop is over the bodies (i=1..m) in our code and as an
    *intern loop we have: j=0..P, k=-j..j
    *
    *and the potential is then given by:
    *
    * Phi(x) = sum_{n=0}^{+} sum_{m=-n}^{n} M_n^m O_n^{-m} (x - *p_center)
    *
    */
    void P2M(CellClass* const inPole, const ContainerClass* const inParticles) {


        for(typename ContainerClass::ConstBasicIterator iterParticle(*inParticles);
	    iterParticle.hasNotFinished() ; iterParticle.gotoNext()){


            //std::cout << "Working on part " << iterParticle.data()->getPhysicalValue() << "\n";
            //F3DPosition tempPos = iterParticle.data()->getPosition() - inPole->getPosition();
            //ok printf("\tpos_rel.x=%.15e\tpos_rel.y=%.15e\tpos_rel.z=%.15e\n",tempPos.getX(),tempPos.getY(),tempPos.getZ());
            //ok printf("\tp_center.x=%.15e\tp_center.y=%.15e\tp_center.z=%.15e\n",inPole->getPosition().getX(),inPole->getPosition().getY(),inPole->getPosition().getZ());
            //ok printf("\tbody.x=%.15e\tbody.y=%.15e\tbody.z=%.15e\n",iterParticle.data()->getPosition().getX(),iterParticle.data()->getPosition().getY(),iterParticle.data()->getPosition().getZ());

            harmonicInner(positionTsmphere(iterParticle.data().getPosition() - inPole->getPosition()),current_thread_Y);

            //ok printf("\tr=%.15e\tcos_theta=%.15e\tsin_theta=%.15e\tphi=%.15e\n",spherical.r,spherical.cosTheta,spherical.sinTheta,spherical.phi);

            FComplexe* p_exp_term = inPole->getMultipole();
            FComplexe* p_Y_term   = current_thread_Y;
            FReal pow_of_minus_1_j = 1.0;//(-1)^j
            const FReal valueParticle = iterParticle.data().getPhysicalValue();

            for(int j = 0 ; j <= FMB_Info_P ; ++j, pow_of_minus_1_j = -pow_of_minus_1_j ){
                for(int k = 0 ; k <= j ; ++k, ++p_Y_term, ++p_exp_term){
                    p_Y_term->mulRealAndImag( valueParticle * pow_of_minus_1_j );
                    (*p_exp_term) += (*p_Y_term);
                    //printf("\tj=%d\tk=%d\tp_exp_term.real=%.15e\tp_exp_term.imag=%.15e\tp_Y_term.real=%.15e\tp_Y_term.imag=%.15e\tpow_of_minus_1_j=%.15e\n",
                    //       j,k,(*p_exp_term).getReal(),(*p_exp_term).getImag(),(*p_Y_term).getReal(),(*p_Y_term).getImag(),pow_of_minus_1_j);
                }
            }
        }

         
    }

    /**
    *-----------------------------------
    *octree_Upward_pass_internal_cell
    *expansion_M2M_add
    *-----------------------------------
    *We compute the translation of multipole_exp_src from *p_center_of_exp_src to
    *p_center_of_exp_target, and add the result to multipole_exp_target.
    *
    * O_n^l (with n=0..P, l=-n..n) being the former multipole expansion terms
    * (whose center is *p_center_of_multipole_exp_src) we have for the new multipole
    * expansion terms (whose center is *p_center_of_multipole_exp_target):

    * M_j^k = sum{n=0..j}
    * sum{l=-n..n, |k-l|<=j-n}
    * O_n^l Inner_{j-n}^{k-l}(rho, alpha, beta)
    *
    * where (rho, alpha, beta) are the spherical coordinates of the vector :
    * p_center_of_multipole_exp_target - *p_center_of_multipole_exp_src
    *
    * Warning: if j-n < |k-l| we do nothing.
     */
    void M2M(CellClass* const FRestrict inPole, const CellClass *const FRestrict *const FRestrict inChild, const int inLevel) {


        // We do NOT have: for(l=n-j+k; l<=j-n+k ;++l){} <=> for(l=-n; l<=n ;++l){if (j-n >= abs(k-l)){}}
        //     But we have:  for(k=MAX(0,n-j+l); k<=j-n+l; ++k){} <=> for(k=0; k<=j; ++k){if (j-n >= abs(k-l)){}}
        //     (This is not the same as in L2L since the range of values of k is not the same, compared to "n-j" or "j-n".)
        //     Therefore the loops over n and l are the outmost ones and
        //     we invert the loop over j with the summation with n:
        //     for{j=0..P} sum_{n=0}^j <-> sum_{n=0}^P for{j=n..P}
        FComplexe* const multipole_exp_target = inPole->getMultipole();
        //printf("Morton = %lld\n",inPole->getMortonIndex());

        for(int idxChild = 0 ; idxChild < 8 ; ++idxChild){
            if(!inChild[idxChild]) continue;
            //printf("\tChild %d\n",idxChild);

            const FComplexe* const multipole_exp_src = inChild[idxChild]->getMultipole();

            const FComplexe* const M2M_transfer = transitionM2M[inLevel][idxChild];

            for(int n = 0 ; n <= FMB_Info_P ; ++n ){
                // l<0 // (-1)^l
                FReal pow_of_minus_1_for_l = static_cast<FReal>( n % 2 ? -1.0 : 1.0);

                // O_n^l : here points on the source multipole expansion term of degree n and order |l|
                const FComplexe* p_src_exp_term = multipole_exp_src + expansion_Redirection_array_for_j[n]+n;
                //printf("\t[p_src_exp_term] expansion_Redirection_array_for_j[n]=%d\tn=%d\n",expansion_Redirection_array_for_j[n],n);

                int l = -n;
                for(; l<0 ; ++l, --p_src_exp_term, pow_of_minus_1_for_l = -pow_of_minus_1_for_l){

                    for(int j = n ; j<= FMB_Info_P ; ++j ){
                        // M_j^k
                        FComplexe *p_target_exp_term = multipole_exp_target + expansion_Redirection_array_for_j[j];
                        //printf("\t[p_target_exp_term] expansion_Redirection_array_for_j[j]=%d\n",expansion_Redirection_array_for_j[j]);
                        // Inner_{j-n}^{k-l} : here points on the M2M transfer function/expansion term of degree n-j and order |k-l|
                        const FComplexe *p_Inner_term= M2M_transfer + expansion_Redirection_array_for_j[j-n]-l /* k==0 */;
                        //printf("\t[p_Inner_term] expansion_Redirection_array_for_j[j-n]=%d\tl=%d\n",expansion_Redirection_array_for_j[j-n],-l);

                        // since n-j+l<0
                        for(int k=0 ; k <= (j-n+l) ; ++k, ++p_target_exp_term, ++p_Inner_term){ // l<0 && k>=0 => k-l>0
                            p_target_exp_term->incReal( pow_of_minus_1_for_l *
                                                        ((p_src_exp_term->getReal() * p_Inner_term->getReal()) +
                                                         (p_src_exp_term->getImag() * p_Inner_term->getImag())));
                            p_target_exp_term->incImag( pow_of_minus_1_for_l *
                                                        ((p_src_exp_term->getReal() * p_Inner_term->getImag()) -
                                                         (p_src_exp_term->getImag() * p_Inner_term->getReal())));
                            /*printf("1\n");
                            printf("\tp_src_exp_term->dat[REAL]=%.15e\tp_src_exp_term->dat[IMAG]=%.15e\n", p_src_exp_term->getReal(),p_src_exp_term->getImag());
                            printf("\tp_Inner_term->dat[REAL]=%.15e\tp_Inner_term->dat[IMAG]=%.15e\n", p_Inner_term->getReal(),p_Inner_term->getImag());
                            printf("\tn[%d]l[%d]j[%d]k[%d] = %.15e / %.15e\n",
                                   n,l,j,k,
                                   p_target_exp_term->getReal(),p_target_exp_term->getImag());*/
                        } // for k
                    } // for j
                } // for l

                // l>=0
                for(; l <= n ; ++l, ++p_src_exp_term, pow_of_minus_1_for_l = -pow_of_minus_1_for_l){

                    for( int j=n ; j <= FMB_Info_P ; ++j ){
                        // (-1)^k
                        FReal pow_of_minus_1_for_k = static_cast<FReal>( FMath::Max(0,n-j+l) %2 ? -1.0 : 1.0 );
                        // M_j^k
                        FComplexe *p_target_exp_term = multipole_exp_target + expansion_Redirection_array_for_j[j] + FMath::Max(0,n-j+l);
                        // Inner_{j-n}^{k-l} : here points on the M2M transfer function/expansion term of degree n-j and order |k-l|
                        const FComplexe *p_Inner_term = M2M_transfer + expansion_Redirection_array_for_j[j-n] + l - FMath::Max(0,n-j+l);// -(k-l)

                        int k = FMath::Max(0,n-j+l);
                        for(; k <= (j-n+l) && (k-l) < 0 ; ++k, ++p_target_exp_term, --p_Inner_term, pow_of_minus_1_for_k = -pow_of_minus_1_for_k){ /* l>=0 && k-l<0 */
                            p_target_exp_term->incReal( pow_of_minus_1_for_k * pow_of_minus_1_for_l *
                                                        ((p_src_exp_term->getReal() * p_Inner_term->getReal()) +
                                                         (p_src_exp_term->getImag() * p_Inner_term->getImag())));
                            p_target_exp_term->incImag(pow_of_minus_1_for_k * pow_of_minus_1_for_l *
                                                       ((p_src_exp_term->getImag() * p_Inner_term->getReal()) -
                                                        (p_src_exp_term->getReal() * p_Inner_term->getImag())));
                            /*printf("2\n");
                            printf("\tp_src_exp_term->dat[REAL]=%.15e\tp_src_exp_term->dat[IMAG]=%.15e\n", p_src_exp_term->getReal(),p_src_exp_term->getImag());
                            printf("\tp_Inner_term->dat[REAL]=%.15e\tp_Inner_term->dat[IMAG]=%.15e\n", p_Inner_term->getReal(),p_Inner_term->getImag());
                            printf("\tn[%d]l[%d]j[%d]k[%d] = %.15e / %.15e\n",
                                   n,l,j,k,
                                   p_target_exp_term->getReal(),p_target_exp_term->getImag());*/
                        } // for k

                        for(; k <= (j - n + l) ; ++k, ++p_target_exp_term, ++p_Inner_term){ // l>=0 && k-l>=0
                            p_target_exp_term->incReal(
                                    (p_src_exp_term->getReal() * p_Inner_term->getReal()) -
                                    (p_src_exp_term->getImag() * p_Inner_term->getImag()));
                            p_target_exp_term->incImag(
                                    (p_src_exp_term->getImag() * p_Inner_term->getReal()) +
                                    (p_src_exp_term->getReal() * p_Inner_term->getImag()));
                            /*printf("3\n");
                            printf("\tp_src_exp_term->dat[REAL]=%.15e\tp_src_exp_term->dat[IMAG]=%.15e\n", p_src_exp_term->getReal(),p_src_exp_term->getImag());
                            printf("\tp_Inner_term->dat[REAL]=%.15e\tp_Inner_term->dat[IMAG]=%.15e\n", p_Inner_term->getReal(),p_Inner_term->getImag());
                            printf("\tn[%d]l[%d]j[%d]k[%d] = %.15e / %.15e\n",
                                   n,l,j,k,
                                   p_target_exp_term->getReal(),p_target_exp_term->getImag());*/

                        } // for k
                    } // for j
                } // for l
            } // for n
        }

        /*for(int idxPole = 0 ; idxPole < FMB_Info_M2L_exp_size ; ++idxPole){
            printf("[%d] real %.15e imag %.15e\n",
                   idxPole,
                   multipole_exp_target[idxPole].getReal(),
                   multipole_exp_target[idxPole].getImag());
        }*/

         
    }

    void convert_exp2nexp_inplace(FComplexe* const exp){
        int j, k;
        const int P = FMB_Info_P;
        FReal pow_of_minus_1_for_k;
        FComplexe* p_exp  = NULL;
        FComplexe* p_nexp = NULL;

        for (j=P; j>=0; --j){
            /* Position in 'exp':  (j*(j+1)*0.5) + k
         * Position in 'nexp':  j*(j+1)      + k
         */
            int jj_plus_1      = j*(j+1);
            int half_jj_plus_1 = int(FReal(jj_plus_1)*0.5);
            p_exp  = exp + half_jj_plus_1 + j;
            p_nexp = exp + jj_plus_1      + j;

            /* Positive (or null) orders: */
            for (k=j; k>=0; --k){
                *p_nexp = *p_exp;
                //printf("[%d] real = %.15e imag %.15e\n",p_nexp-exp,p_nexp->getReal(),p_nexp->getImag());
                --p_exp;
                --p_nexp;
            } /* for k */

            /* Negative orders: */
            p_exp  += j+1 + half_jj_plus_1;
            p_nexp -= j-1;
            for (k= -j,
                 pow_of_minus_1_for_k = static_cast<FReal>((j%2) ? -1.0 : 1.0);
            k<0;
            ++k,
            pow_of_minus_1_for_k = -pow_of_minus_1_for_k){
                p_nexp->setReal(pow_of_minus_1_for_k * p_exp->getReal());
                p_nexp->setImag((-pow_of_minus_1_for_k) * p_exp->getImag());
                //printf("[%d] real = %.15e imag %.15e\n",p_nexp-exp,p_nexp->getReal(),p_nexp->getImag());
                --p_exp;
                ++p_nexp;
            } /* for k */

        } /* j */
    }

    /////////////////////////////////////////////////////////////////////////////////
    //    M2L
    /////////////////////////////////////////////////////////////////////////////////

    void ff_block_matrix_Product(FComplexe* local_exp,
                                 const FComplexe* multipole_nexp,
                                 block_matrix_t* p_transfer_block_matrix){

        int P = p_transfer_block_matrix->P_block;
        int M = p_transfer_block_matrix->M_block;
        int N = p_transfer_block_matrix->N_block;

        FComplexe exp_term_tmp;

        const FReal alpha_and_beta[2] = {1.0,0.0};

        /*    fprintf(stdout, "ff_block_matrix_Product() with P=%i, M=%i, N=%i\n", P, M, N); */

        /* "p_transfer_block_matrix->below == NULL" implies
           * "p_transfer_block_matrix->right == NULL" too.
           *
           * We can also test: " if (ff_block_matrix_Is_terminal_case(M, N, P, FMB_Info.stop_for_block)) "
           */
        if (p_transfer_block_matrix->below == NULL) {
            /*************** Treat terminal case: ***************/
            int row_nb ;
            int column_nb ;
            int band_number;
            FComplexe *data = p_transfer_block_matrix->data;
            int P_bn_1; /* will contain ' P - band_number +1 ' */

            /* We do a product with "bands": there is P+1 bands: */

            /*      HPMSTART_DETAILED(60, "M2L blas BLOCK: terminale case");  */

            /* First band (band_number == 0): a level 1 BLAS can be faster: */
            row_nb = M+1;
            column_nb = (P+1)*(P+1 + 2*N); /* see below */

            if (row_nb == 1){  /* level 1 BLAS is faster */

                cblas_dotu_sub<FReal>(column_nb,
                                data, 1,
                                multipole_nexp, 1,
                                &exp_term_tmp);
                local_exp->incReal(exp_term_tmp.getReal());
                local_exp->incImag(exp_term_tmp.getImag());
            }
            else {
                cblas_gemv<FReal>(CblasColMajor, CblasTrans,
                                  column_nb, row_nb,
                                      alpha_and_beta, data, column_nb,
                                      multipole_nexp, 1,
                                      alpha_and_beta, local_exp, 1);
            }

            data += row_nb * column_nb; /* set data to start of next band: see below */
            local_exp += row_nb; /* update 'local_exp' */


            /* Other bands:
             * the bands are stored one after the other, starting from p_ff_transfer_matrix->data,
             * so that the leading dimension is the smallest one */
            P_bn_1 = P;
            for (band_number=1;
                 band_number<=P;
                 ++band_number){
                row_nb =  M + band_number + 1;
                column_nb = (P_bn_1) * (P_bn_1 + 2*N); /* sum_{k=0}^{P-band_number} (2*(N+k) +1)
                                                  * = sum_{k=0}^{P-band_number} (2*k+1) + 2*N*(P-band_number+1)
                                                  * = (P-band_number+1)(P-band_number+1 + 2*N) */
                cblas_gemv<FReal>(CblasColMajor, CblasTrans,
                                  column_nb, row_nb,
                                      alpha_and_beta, data, column_nb,
                                      multipole_nexp, 1,
                                      alpha_and_beta, local_exp, 1);

                data += row_nb * column_nb;  /* set data to start of next band */
                local_exp += row_nb; /* update 'local_exp' */
                --P_bn_1;
            }

            /*     HPMSTOP_DETAILED(60);  */
        }
        else {
            /*************** Recursive case: ***************/
            /* Matrix-vector product with the biggest square sub-matrix
             * ('square' in the number of
             * blocks, in each dimension, it contains). */
            /* bssm: biggest square sub-matrix */
            const int bssm_row_dim = p_transfer_block_matrix->bssm_row_dim;
            const int bssm_column_dim = p_transfer_block_matrix->bssm_column_dim;

            /*     HPMSTART_DETAILED(61, "M2L blas BLOCK: bssm");  */

            cblas_gemv<FReal>(CblasColMajor, CblasTrans,
                              bssm_column_dim, bssm_row_dim,
                                  alpha_and_beta, p_transfer_block_matrix->data, bssm_column_dim,
                                  multipole_nexp, 1,
                                  alpha_and_beta, local_exp, 1);

            /*     HPMSTOP_DETAILED(61);  */

            /* Recursive calls: */
            /* IMPORTANT: we perform all the products with the
             * sub-matrices on the right before performing calls for
             * sub-matrices below, since we will hence browse only once
             * the "local exp vector" (for writing) and browse several
             * times the "multipole exp vector" (but only for reading). */
            /* Sub-matrix "on the right": */
            ff_block_matrix_Product(local_exp,
                                    multipole_nexp + bssm_column_dim,
                                    p_transfer_block_matrix->right);

            /* Sub-matrix "below": */
            ff_block_matrix_Product(local_exp + bssm_row_dim,
                                    multipole_nexp,
                                    p_transfer_block_matrix->below);


        }
    }

    /**
    *------------------
    * ff_block_matrix_Product
    * ff_matrix_Product
    * octree_Compute_local_exp_M2L
    *-------------------
    *We compute the conversion of multipole_exp_src in *p_center_of_exp_src to
    *a local expansion in *p_center_of_exp_target, and add the result to local_exp_target.
    *
    *O_n^l (with n=0..P, l=-n..n) being the former multipole expansion terms
    *(whose center is *p_center_of_multipole_exp_src) we have for the new local
    *expansion terms (whose center is *p_center_of_local_exp_target):
    *
    *L_j^k = sum{n=0..+}
    *sum{l=-n..n}
    *O_n^l Outer_{j+n}^{-k-l}(rho, alpha, beta)
    *
    *where (rho, alpha, beta) are the spherical coordinates of the vector :
    *p_center_of_local_exp_src - *p_center_of_multipole_exp_target
    *
    *Remark: here we have always j+n >= |-k-l|
      *
      */
    void M2L(CellClass* const FRestrict pole, const CellClass* distantNeighbors[189],
             const int size, const int inLevel) {


        const FTreeCoordinate& coordCenter = pole->getCoordinate();
        for(int idxSize = 0 ; idxSize < size ; ++idxSize){
            const FTreeCoordinate& coordNeighbors = distantNeighbors[idxSize]->getCoordinate();

            //printf("Morton = %lld\n",pole->getMortonIndex());
            //printf("\tMorton Neighbors = %lld\n",distantNeighbors[idxSize]->getMortonIndex());
            //printf("\tidxSize = %d\tleve = %d\tMorton = %lld\n",idxSize,inLevel,distantNeighbors[idxSize]->getMortonIndex());

            block_matrix_t* const transfer_M2L_matrix = transferM2L[inLevel]
                                                         [(coordCenter.getX()+halphSize1Dim-coordNeighbors.getX())]
                                                         [(coordCenter.getY()+halphSize1Dim-coordNeighbors.getY())]
                                                         [(coordCenter.getZ()+halphSize1Dim-coordNeighbors.getZ())];
            //printf("level = %d\tx=%ld\ty=%ld\tz=%ld\n", inLevel,
            //       (coordCenter.getX()-coordNeighbors.getX()),
            //       (coordCenter.getY()-coordNeighbors.getY()),
            //       (coordCenter.getZ()-coordNeighbors.getZ()));
            /*printf("M2L_transfer[0]= %.15e/%.15e\n",M2L_transfer->getReal(),M2L_transfer->getImag());
            printf("M2L_transfer[1]= %.15e/%.15e\n",M2L_transfer[1].getReal(),M2L_transfer[1].getImag());
            printf("M2L_transfer[2]= %.15e/%.15e\n",M2L_transfer[2].getReal(),M2L_transfer[2].getImag());*/
            const FComplexe* const multipole_exp_src = distantNeighbors[idxSize]->getMultipole();
            FComplexe* p_target_exp_term = pole->getLocal();

            FComplexe multipole_exp_src_changed[FF_MATRIX_COLUMN_DIM];
            memcpy(multipole_exp_src_changed,multipole_exp_src,sizeof(FComplexe)*FF_MATRIX_ROW_DIM);

            convert_exp2nexp_inplace(multipole_exp_src_changed);

            ff_block_matrix_Product(p_target_exp_term,multipole_exp_src_changed,transfer_M2L_matrix);

            /*for(int j = 0 ; j < FF_MATRIX_COLUMN_DIM  ; ++j){
                printf("\t\t multipole_nexp[%d] real = %.15e imag = %.15e\n",
                       j,
                       ((double*)multipole_exp_src_changed)[2*j],
                       ((double*)multipole_exp_src_changed)[2*j + 1]);
            }*/
        }

         
    }

    /////////////////////////////////////////////////////////////////////////////////
    //    Downard
    /////////////////////////////////////////////////////////////////////////////////

    /** expansion_L2L_add
      *We compute the shift of local_exp_src from *p_center_of_exp_src to
      *p_center_of_exp_target, and set the result to local_exp_target.
      *
      *O_n^l (with n=0..P, l=-n..n) being the former local expansion terms
      *(whose center is *p_center_of_exp_src) we have for the new local
      *expansion terms (whose center is *p_center_of_exp_target):
      *
      *L_j^k = sum{n=j..P}
      *sum{l=-n..n}
      *O_n^l Inner_{n-j}^{l-k}(rho, alpha, beta)
      *
      *where (rho, alpha, beta) are the spherical coordinates of the vector :
      *p_center_of_exp_target - *p_center_of_exp_src
      *
      *Warning: if |l-k| > n-j, we do nothing.
      */
    void L2L(const CellClass* const FRestrict pole, CellClass* FRestrict *const FRestrict child, const int inLevel) {


        for(int idxChild = 0 ; idxChild < 8 ; ++idxChild){
            // if no child at this position
            if(!child[idxChild]) continue;

            const FComplexe* const L2L_tranfer = transitionL2L[inLevel][idxChild];
            const FComplexe* const local_exp_src = pole->getLocal();
            FComplexe* const local_exp_target = child[idxChild]->getLocal();

            //printf("Level %d\n", inLevel);
            //printf("Father morton %lld\n", pole->getMortonIndex());
            //printf("Child morton %lld\n", child[idxChild]->getMortonIndex());

            //printf("local exp target %h\n", local_exp_target);

            // L_j^k
            FComplexe* p_target_exp_term = local_exp_target;
            for (int j=0 ; j<= FMB_Info_P ; ++j){
                // (-1)^k
                FReal pow_of_minus_1_for_k = 1.0;
                for (int k=0 ; k <= j ; ++k, pow_of_minus_1_for_k = -pow_of_minus_1_for_k, ++p_target_exp_term){
                    for (int n=j; n<=FMB_Info_P;++n){
                        // O_n^l : here points on the source multipole expansion term of degree n and order |l|
                        const FComplexe* p_src_exp_term = local_exp_src + expansion_Redirection_array_for_j[n] + n-j+k;
                        //printf("expansion_Redirection_array_for_j[n] + n-j+k %d\n", expansion_Redirection_array_for_j[n] + n-j+k);
                        int l = n-j+k;
                        // Inner_{n-j}^{l-k} : here points on the L2L transfer function/expansion term of degree n-j and order |l-k|
                        const FComplexe* p_Inner_term = L2L_tranfer + expansion_Redirection_array_for_j[n-j] + l-k;

                        //printf("1\n");
                        for ( ; l-k>0;  --l, --p_src_exp_term, --p_Inner_term){ /* l>0 && l-k>0 */
                            p_target_exp_term->incReal( (p_src_exp_term->getReal() * p_Inner_term->getReal()) -
                                                        (p_src_exp_term->getImag() * p_Inner_term->getImag()));
                            p_target_exp_term->incImag( (p_src_exp_term->getImag() * p_Inner_term->getReal()) +
                                                        (p_src_exp_term->getReal() * p_Inner_term->getImag()));
                            /*printf("\t p_src_exp_term->real = %lf \t p_src_exp_term->imag = %lf \n",
                                       p_src_exp_term->getReal(),p_src_exp_term->getImag());
                                printf("\t p_Inner_term->real = %lf \t p_Inner_term->imag = %lf \n",
                                       p_Inner_term->getReal(),p_Inner_term->getImag());
                                printf("\t\t p_target_exp_term->real = %lf \t p_target_exp_term->imag = %lf \n",
                                       p_target_exp_term->getReal(),p_target_exp_term->getImag());
                                printf("\tp_target_exp_term = %d\n",p_target_exp_term-local_exp_target);*/
                        }

                        //printf("2\n");
                        // (-1)^l
                        FReal pow_of_minus_1_for_l = static_cast<FReal>((l%2) ? -1.0 : 1.0);
                        for (; l>0 && l>=j-n+k; --l, pow_of_minus_1_for_l = -pow_of_minus_1_for_l, --p_src_exp_term, ++p_Inner_term){ /* l>0 && l-k<=0 */
                            p_target_exp_term->incReal( pow_of_minus_1_for_l * pow_of_minus_1_for_k *
                                                        ((p_src_exp_term->getReal() * p_Inner_term->getReal()) +
                                                         (p_src_exp_term->getImag() * p_Inner_term->getImag())));
                            p_target_exp_term->incImag( pow_of_minus_1_for_l * pow_of_minus_1_for_k *
                                                        ((p_src_exp_term->getImag() * p_Inner_term->getReal()) -
                                                         (p_src_exp_term->getReal() * p_Inner_term->getImag())));
                            /*printf("\t p_src_exp_term->real = %lf \t p_src_exp_term->imag = %lf \n",
                                       p_src_exp_term->getReal(),p_src_exp_term->getImag());
                                printf("\t p_Inner_term->real = %lf \t p_Inner_term->imag = %lf \n",
                                       p_Inner_term->getReal(),p_Inner_term->getImag());
                                printf("\t\t p_target_exp_term->real = %lf \t p_target_exp_term->imag = %lf \n",
                                       p_target_exp_term->getReal(),p_target_exp_term->getImag());
                                printf("\tp_target_exp_term = %d\n",p_target_exp_term-local_exp_target);*/
                        }

                        //printf("3\n");
                        // l<=0 && l-k<=0
                        for (; l>=j-n+k; --l, ++p_src_exp_term, ++p_Inner_term){
                            p_target_exp_term->incReal( pow_of_minus_1_for_k *
                                                        ((p_src_exp_term->getReal() * p_Inner_term->getReal()) -
                                                         (p_src_exp_term->getImag() * p_Inner_term->getImag())));
                            p_target_exp_term->decImag( pow_of_minus_1_for_k *
                                                        ((p_src_exp_term->getImag() * p_Inner_term->getReal()) +
                                                         (p_src_exp_term->getReal() * p_Inner_term->getImag())));
                            /*printf("\t p_src_exp_term->real = %lf \t p_src_exp_term->imag = %lf \n",
                                       p_src_exp_term->getReal(),p_src_exp_term->getImag());
                                printf("\t p_Inner_term->real = %lf \t p_Inner_term->imag = %lf \n",
                                       p_Inner_term->getReal(),p_Inner_term->getImag());
                                printf("\t\t p_target_exp_term->real = %lf \t p_target_exp_term->imag = %lf \n",
                                       p_target_exp_term->getReal(),p_target_exp_term->getImag());
                                printf("\tj=%d\tk=%d\tn=%d\tl=%d\tpow_of_minus_1_for_k=%.15e\n",j,k,n,l,pow_of_minus_1_for_k);
                                printf("\tp_target_exp_term = %d\n",p_target_exp_term-local_exp_target);*/
                        }
                        /*printf("\tj=%d\tk=%d\tn=%d\tl=%d\n",j,k,n,l);
                        printf("\t\t p_target_exp_term->real = %lf \t p_target_exp_term->imag = %lf \n",
                               p_target_exp_term->getReal(),p_target_exp_term->getImag());*/
                    }
                }
            }
        }
         
    }


    /** bodies_L2P
      *     expansion_L2P_add_to_force_vector_and_to_potential
      *         expansion_L2P_add_to_force_vector
      *         expansion_Evaluate_local_with_Y_already_computed
      */
    void L2P(const CellClass* const local, ContainerClass* const particles){

        typename ContainerClass::BasicIterator iterTarget(*particles);
        while( iterTarget.hasNotFinished() ){
            //printf("Morton %lld\n",local->getMortonIndex());

            F3DPosition force_vector_in_local_base;
            typename FFmbKernelsBlockBlas<ParticleClass,CellClass,ContainerClass>::Spherical spherical;
            spherical = positionTsmphere( iterTarget.data().getPosition() - local->getPosition());

            /*printf("\t\t bodies_it_Get_p_position(&it) x = %lf \t y = %lf \t z = %lf \n",
                   (iterTarget.data()->getPosition()).getX(),
                   (iterTarget.data()->getPosition()).getY(),
                   (iterTarget.data()->getPosition()).getZ());
            printf("\t\t p_leaf_center x = %lf \t y = %lf \t z = %lf \n",
                   (local->getPosition()).getX(),
                   (local->getPosition()).getY(),
                   (local->getPosition()).getZ());*/
            /*printf("\t\t p_position_to_leaf_center x = %lf \t y = %lf \t z = %lf \n",
                    (iterTarget.data()->getPosition() - local->getPosition()).getX(),
                    (iterTarget.data()->getPosition() - local->getPosition()).getY(),
                    (iterTarget.data()->getPosition() - local->getPosition()).getZ());*/
            /*printf("\t\t phi = %lf \t cos = %lf \t sin = %lf \t r= %lf \n",
                    spherical.phi,spherical.cosTheta,spherical.sinTheta,spherical.r);*/

            harmonicInnerThetaDerivated( spherical, FFmbKernelsBlockBlas<ParticleClass,CellClass,ContainerClass>::current_thread_Y, FFmbKernelsBlockBlas<ParticleClass,CellClass,ContainerClass>::current_thread_Y_theta_derivated);

            // The maximum degree used here will be P.
            const FComplexe* p_Y_term = FFmbKernelsBlockBlas<ParticleClass,CellClass,ContainerClass>::current_thread_Y+1;
            const FComplexe* p_Y_theta_derivated_term = FFmbKernelsBlockBlas<ParticleClass,CellClass,ContainerClass>::current_thread_Y_theta_derivated+1;
            const FComplexe* p_local_exp_term = local->getLocal()+1;

            for (int j = 1 ; j <= FMB_Info_P ; ++j ){
                FComplexe exp_term_aux;

                // k=0:
                // F_r:
                exp_term_aux.setReal( (p_Y_term->getReal() * p_local_exp_term->getReal()) - (p_Y_term->getImag() * p_local_exp_term->getImag()) );
                exp_term_aux.setImag( (p_Y_term->getReal() * p_local_exp_term->getImag()) + (p_Y_term->getImag() * p_local_exp_term->getReal()) );

                force_vector_in_local_base.setX( force_vector_in_local_base.getX() + FReal(j) * exp_term_aux.getReal());
                // F_phi: k=0 => nothing to do for F_phi
                // F_theta:
                exp_term_aux.setReal( (p_Y_theta_derivated_term->getReal() * p_local_exp_term->getReal()) - (p_Y_theta_derivated_term->getImag() * p_local_exp_term->getImag()) );
                exp_term_aux.setImag( (p_Y_theta_derivated_term->getReal() * p_local_exp_term->getImag()) + (p_Y_theta_derivated_term->getImag() * p_local_exp_term->getReal()) );

                force_vector_in_local_base.setY( force_vector_in_local_base.getY() + exp_term_aux.getReal());


                /*printf("\t j = %d \t exp_term_aux real = %lf imag = %lf \n",
                                                        j,
                                                        exp_term_aux.getReal(),
                                                        exp_term_aux.getImag());*/
                /*printf("\t\t\t p_Y_theta_derivated_term->getReal = %lf \t p_Y_theta_derivated_term->getImag = %lf \n",
                       p_Y_theta_derivated_term->getReal(),p_Y_theta_derivated_term->getImag());*/
                //printf("\t\t\t p_local_exp_term->getReal = %lf \t p_local_exp_term->getImag = %lf \n",
                //       p_local_exp_term->getReal(),p_local_exp_term->getImag());
                //printf("\t\t\t p_Y_term->getReal = %lf \t p_Y_term->getImag = %lf \n",p_Y_term->getReal(),p_Y_term->getImag());

                //printf("[LOOP1] force_vector_in_local_base x = %lf \t y = %lf \t z = %lf \n",
                //       force_vector_in_local_base.getX(),force_vector_in_local_base.getY(),force_vector_in_local_base.getZ());


                ++p_local_exp_term;
                ++p_Y_term;
                ++p_Y_theta_derivated_term;


                // k>0:
                for (int k=1; k<=j ;++k, ++p_local_exp_term, ++p_Y_term, ++p_Y_theta_derivated_term){
                    // F_r:

                    exp_term_aux.setReal( (p_Y_term->getReal() * p_local_exp_term->getReal()) - (p_Y_term->getImag() * p_local_exp_term->getImag()) );
                    exp_term_aux.setImag( (p_Y_term->getReal() * p_local_exp_term->getImag()) + (p_Y_term->getImag() * p_local_exp_term->getReal()) );

                    force_vector_in_local_base.setX(force_vector_in_local_base.getX() + FReal(2 * j) * exp_term_aux.getReal());
                    // F_phi:
                    force_vector_in_local_base.setZ( force_vector_in_local_base.getZ() - FReal(2 * k) * exp_term_aux.getImag());
                    // F_theta:

                    /*printf("\t\t k = %d \t j = %d \t exp_term_aux real = %.15e imag = %.15e \n",
                                                            k,j,
                                                            exp_term_aux.getReal(),
                                                            exp_term_aux.getImag());*/
                    //printf("\t\t\t p_Y_term->getReal = %.15e \t p_Y_term->getImag = %.15e \n",p_Y_term->getReal(),p_Y_term->getImag());
                    //printf("\t\t\t p_local_exp_term->getReal = %.15e \t p_local_exp_term->getImag = %.15e \n",p_local_exp_term->getReal(),p_local_exp_term->getImag());

                    exp_term_aux.setReal( (p_Y_theta_derivated_term->getReal() * p_local_exp_term->getReal()) - (p_Y_theta_derivated_term->getImag() * p_local_exp_term->getImag()) );
                    exp_term_aux.setImag( (p_Y_theta_derivated_term->getReal() * p_local_exp_term->getImag()) + (p_Y_theta_derivated_term->getImag() * p_local_exp_term->getReal()) );

                    force_vector_in_local_base.setY(force_vector_in_local_base.getY() + 2 * exp_term_aux.getReal());

                    /*printf("\t\t k = %d \t j = %d \t exp_term_aux real = %lf imag = %lf \n",
                                                            k,j,
                                                            exp_term_aux.getReal(),
                                                            exp_term_aux.getImag());*/
                    /*printf("\t\t\t p_Y_theta_derivated_term->getReal = %lf \t p_Y_theta_derivated_term->getImag = %lf \n",
                           p_Y_theta_derivated_term->getReal(),p_Y_theta_derivated_term->getImag());*/
                    /*printf("\t\t\t p_local_exp_term->getReal = %lf \t p_local_exp_term->getImag = %lf \n",
                           p_local_exp_term->getReal(),p_local_exp_term->getImag());*/

                    /*printf("[LOOP2] force_vector_in_local_base x = %lf \t y = %lf \t z = %lf \n",
                           force_vector_in_local_base.getX(),force_vector_in_local_base.getY(),force_vector_in_local_base.getZ());*/
                }
            }

            /*printf("[END LOOP] force_vector_in_local_base x = %lf \t y = %lf \t z = %lf \n",
                   force_vector_in_local_base.getX(),force_vector_in_local_base.getY(),force_vector_in_local_base.getZ());*/

            // We want: - gradient(POTENTIAL_SIGN potential).
            // The -(- 1.0) computing is not the most efficient programming ...
            //#define FMB_TMP_SIGN -(POTENTIAL_SIGN 1.0)
            force_vector_in_local_base.setX( force_vector_in_local_base.getX() * FReal(-1.0) / spherical.r);
            force_vector_in_local_base.setY( force_vector_in_local_base.getY() * FReal(-1.0) / spherical.r);
            force_vector_in_local_base.setZ( force_vector_in_local_base.getZ() * FReal(-1.0) / (spherical.r * spherical.sinTheta));
            //#undef FMB_TMP_SIGN

            /////////////////////////////////////////////////////////////////////

            //spherical_position_Set_ph
            //FMB_INLINE COORDINATES_T angle_Convert_in_MinusPi_Pi(COORDINATES_T a){
            FReal ph = FMath::Fmod(spherical.phi, 2*FMath::FPi);
            if (ph > M_PI) ph -= 2*FMath::FPi;
            if (ph < -M_PI + FMath::Epsilon)  ph += 2 * FMath::Epsilon;

            //spherical_position_Set_th
            FReal th = FMath::Fmod(FMath::ACos(spherical.cosTheta), 2*FMath::FPi);
            if (th < 0.0) th += 2*FMath::FPi;
            if (th > FMath::FPi){
                th = 2*FMath::FPi - th;
                //spherical_position_Set_ph(p, spherical_position_Get_ph(p) + M_PI);
                ph = FMath::Fmod(ph + FMath::FPi, 2*FMath::FPi);
                if (ph > M_PI) ph -= 2*FMath::FPi;
                if (ph < -M_PI + FMath::Epsilon)  ph += 2 * FMath::Epsilon;
                th = FMath::Fmod(th, 2*FMath::FPi);
                if (th > M_PI) th -= 2*FMath::FPi;
                if (th < -M_PI + FMath::Epsilon)  th += 2 * FMath::Epsilon;
            }
            //spherical_position_Set_r
            //FReal rh = spherical.r;
            if (spherical.r < 0){
                //rh = -spherical.r;
                //spherical_position_Set_ph(p, M_PI - spherical_position_Get_th(p));
                ph = FMath::Fmod(FMath::FPi - th, 2*FMath::FPi);
                if (ph > M_PI) ph -= 2*FMath::FPi;
                if (ph < -M_PI + FMath::Epsilon)  ph += 2 * FMath::Epsilon;
                //spherical_position_Set_th(p, spherical_position_Get_th(p) + M_PI);
                th = FMath::Fmod(th + FMath::FPi, 2*FMath::FPi);
                if (th < 0.0) th += 2*FMath::FPi;
                if (th > FMath::FPi){
                    th = 2*FMath::FPi - th;
                    //spherical_position_Set_ph(p, spherical_position_Get_ph(p) + M_PI);
                    ph = FMath::Fmod(ph + FMath::FPi, 2*FMath::FPi);
                    if (ph > M_PI) ph -= 2*FMath::FPi;
                    if (ph < -M_PI + FMath::Epsilon)  ph += 2 * FMath::Epsilon;
                    th = FMath::Fmod(th, 2*FMath::FPi);
                    if (th > M_PI) th -= 2*FMath::FPi;
                    if (th < -M_PI + FMath::Epsilon)  th += 2 * FMath::Epsilon;
                }
            }

            /*printf("[details] ph = %.15e , rh = %.15e , th = %.15e \n",
                   ph,rh,th);*/


            const FReal cos_theta = FMath::Cos(th);
            const FReal cos_phi = FMath::Cos(ph);
            const FReal sin_theta = FMath::Sin(th);
            const FReal sin_phi = FMath::Sin(ph);

            /*printf("[details] cos_theta = %.15e \t cos_phi = %.15e \t sin_theta = %.15e \t sin_phi = %.15e \n",
                   cos_theta, cos_phi, sin_theta, sin_phi);*/
            /*printf("[force_vector_in_local_base] x = %lf \t y = %lf \t z = %lf \n",
                   force_vector_in_local_base.getX(),force_vector_in_local_base.getY(),force_vector_in_local_base.getZ());*/

            F3DPosition force_vector_tmp;

            force_vector_tmp.setX(
                    cos_phi * sin_theta * force_vector_in_local_base.getX() +
                    cos_phi * cos_theta * force_vector_in_local_base.getY() +
                    (-sin_phi) * force_vector_in_local_base.getZ());

            force_vector_tmp.setY(
                    sin_phi * sin_theta * force_vector_in_local_base.getX() +
                    sin_phi * cos_theta * force_vector_in_local_base.getY() +
                    cos_phi * force_vector_in_local_base.getZ());

            force_vector_tmp.setZ(
                    cos_theta * force_vector_in_local_base.getX() +
                    (-sin_theta) * force_vector_in_local_base.getY());

            /*printf("[force_vector_tmp]  = %lf \t y = %lf \t z = %lf \n",
                   force_vector_tmp.getX(),force_vector_tmp.getY(),force_vector_tmp.getZ());*/

            /////////////////////////////////////////////////////////////////////

            //#ifndef _DIRECT_MATRIX_
            // When _DIRECT_MATRIX_ is defined, this multiplication is done in 'leaf_Sum_near_and_far_fields()'
            force_vector_tmp *= iterTarget.data().getPhysicalValue();
            //#endif

            /*printf("[force_vector_tmp] fx = %.15e \t fy = %.15e \t fz = %.15e \n",
                   force_vector_tmp.getX(),force_vector_tmp.getY(),force_vector_tmp.getZ());*/

            iterTarget.data().incForces( force_vector_tmp );

            FReal potential;
            expansion_Evaluate_local_with_Y_already_computed(local->getLocal(),&potential);
            iterTarget.data().setPotential(potential);

            /*printf("[END] fx = %.15e \t fy = %.15e \t fz = %.15e \n\n",
                   iterTarget.data()->getForces().getX(),iterTarget.data()->getForces().getY(),iterTarget.data()->getForces().getZ());*/
            //printf("p_potential = %lf\n", potential);

            iterTarget.gotoNext();
        }
         
    }


    void expansion_Evaluate_local_with_Y_already_computed(const FComplexe* local_exp,
                                                          FReal* const p_result){


        FReal result = 0.0;

        FComplexe* p_Y_term = FFmbKernelsBlockBlas<ParticleClass,CellClass,ContainerClass>::current_thread_Y;
        for(int j = 0 ; j<= FMB_Info_P ; ++j){
            // k=0
            (*p_Y_term) *= (*local_exp);
            result += p_Y_term->getReal();
            //printf("\t\t p_Y_term->real = %.15e p_Y_term->imag = %.15e \t local_exp->real = %.15e local_exp->imag = %.15e \n",
            //       p_Y_term->getReal(), p_Y_term->getImag(), local_exp->getReal(), local_exp->getImag());
            ++p_Y_term;
            ++local_exp;

            // k>0
            for (int k=1; k<=j ;++k, ++p_Y_term, ++local_exp){
                (*p_Y_term) *= (*local_exp);
                result += 2 * p_Y_term->getReal();
                //printf("\t\t p_Y_term->real = %.15e p_Y_term->imag = %.15e \t local_exp->real = %.15e local_exp->imag = %.15e \n",
                //       p_Y_term->getReal(), p_Y_term->getImag(), local_exp->getReal(), local_exp->getImag());
            }
        }

        *p_result = result;

         
    }

    ///////////////////////////////////////////////////////////////////////////////
    // MUTUAL - Need
    ///////////////////////////////////////////////////////////////////////////////


    /** void bodies_Compute_direct_interaction 	(
      *          bodies_t *FMB_RESTRICT  	p_b_target,
      *          bodies_t *FMB_RESTRICT  	p_b_src,
      *          bool  	mutual
      *  )
      *
      */
    void P2P(const MortonIndex inCurrentIndex,
             ContainerClass* const FRestrict targets, const ContainerClass* const FRestrict sources,
             ContainerClass* const directNeighbors[26], const MortonIndex inNeighborsIndex[26], const int size) {

        typename ContainerClass::BasicIterator iterTarget(*targets);
        while( iterTarget.hasNotFinished() ){

            for(int idxDirectNeighbors = 0 ; idxDirectNeighbors < size ; ++idxDirectNeighbors){
                if(inCurrentIndex < inNeighborsIndex[idxDirectNeighbors] ){
                    typename ContainerClass::BasicIterator iterSource(*directNeighbors[idxDirectNeighbors]);
                    while( iterSource.hasNotFinished() ){
                        DIRECT_COMPUTATION_MUTUAL_SOFT(iterTarget.data(),
                                                       iterSource.data());
                        iterSource.gotoNext();
                    }
                }
            }

            typename ContainerClass::BasicIterator iterSameBox = iterTarget;//(*targets);
            iterSameBox.gotoNext();
            while( iterSameBox.hasNotFinished() ){
                if(&iterSameBox.data() < &iterTarget.data()){
                    DIRECT_COMPUTATION_MUTUAL_SOFT(iterTarget.data(),
                                                   iterSameBox.data());
                }
                iterSameBox.gotoNext();
            }

            //printf("x = %e \t y = %e \t z = %e \n",iterTarget.data()->getPosition().getX(),iterTarget.data()->getPosition().getY(),iterTarget.data()->getPosition().getZ());
            //printf("\t P2P fx = %e \t fy = %e \t fz = %e \n",iterTarget.data()->getForces().getX(),iterTarget.data()->getForces().getY(),iterTarget.data()->getForces().getZ());
            //printf("\t potential = %e \n",iterTarget.data()->getPotential());

            iterTarget.gotoNext();
        }
         
    }


    void DIRECT_COMPUTATION_MUTUAL_SOFT(ParticleClass& FRestrict target, ParticleClass& source){

        FReal dx = target.getPosition().getX() - source.getPosition().getX();
        FReal dy = target.getPosition().getY() - source.getPosition().getY();
        FReal dz = target.getPosition().getZ() - source.getPosition().getZ();

        FReal inv_square_distance = FReal(1.0) / (dx*dx + dy*dy + dz*dz + FReal(FMB_Info_eps_soft_square));
        FReal inv_distance = FMath::Sqrt(inv_square_distance);
        inv_distance *= target.getPhysicalValue() * source.getPhysicalValue();
        inv_square_distance *= inv_distance;

        dx *= inv_square_distance;
        dy *= inv_square_distance;
        dz *= inv_square_distance;

        target.incForces(
                dx,
                dy,
                dz
                );
        target.incPotential( inv_distance );

        source.incForces(
                (-dx),
                (-dy),
                (-dz)
                );
        source.incPotential( inv_distance );

         
    }


    ///////////////////////////////////////////////////////////////////////////////
    // NO MUTUAL
    ///////////////////////////////////////////////////////////////////////////////

    /** void bodies_Compute_direct_interaction 	(
      *          bodies_t *FMB_RESTRICT  	p_b_target,
      *          bodies_t *FMB_RESTRICT  	p_b_src,
      *          bool  	mutual
      *  )
      *
      */
    void P2P(ContainerClass* const FRestrict targets, const ContainerClass* const FRestrict sources,
             const ContainerClass* const directNeighbors[26], const int size) {

        typename ContainerClass::BasicIterator iterTarget(*targets);
        while( iterTarget.hasNotFinished() ){

            for(int idxDirectNeighbors = 0 ; idxDirectNeighbors < size ; ++idxDirectNeighbors){
                typename ContainerClass::ConstBasicIterator iterSource(*directNeighbors[idxDirectNeighbors]);
                while( iterSource.hasNotFinished() ){
                    DIRECT_COMPUTATION_NO_MUTUAL_SOFT(iterTarget.data(),
                                                      iterSource.data());
                    iterSource.gotoNext();
                }
            }

            typename ContainerClass::ConstBasicIterator iterSameBox(*sources);
            while( iterSameBox.hasNotFinished() ){
                if(&iterSameBox.data() != &iterTarget.data()){
                    DIRECT_COMPUTATION_NO_MUTUAL_SOFT(iterTarget.data(),
                                                      iterSameBox.data());
                }
                iterSameBox.gotoNext();
            }

            //printf("x = %e \t y = %e \t z = %e \n",iterTarget.data()->getPosition().getX(),iterTarget.data()->getPosition().getY(),iterTarget.data()->getPosition().getZ());
            //printf("\t P2P fx = %e \t fy = %e \t fz = %e \n",iterTarget.data()->getForces().getX(),iterTarget.data()->getForces().getY(),iterTarget.data()->getForces().getZ());
            //printf("\t potential = %e \n",iterTarget.data()->getPotential());

            iterTarget.gotoNext();
        }
         
    }


    void DIRECT_COMPUTATION_NO_MUTUAL_SOFT(ParticleClass& target, const ParticleClass& source){

        const FReal dx = target.getPosition().getX() - source.getPosition().getX();
        const FReal dy = target.getPosition().getY() - source.getPosition().getY();
        const FReal dz = target.getPosition().getZ() - source.getPosition().getZ();

        FReal inv_square_distance = FReal(1.0) / (dx*dx + dy*dy + dz*dz + FReal(FMB_Info_eps_soft_square));
        FReal inv_distance = FMath::Sqrt(inv_square_distance);
        inv_distance *= target.getPhysicalValue() * source.getPhysicalValue();
        inv_square_distance *= inv_distance;

        target.incForces(
                dx * inv_square_distance,
                dy * inv_square_distance,
                dz * inv_square_distance
                );

        target.incPotential( inv_distance );
         
    }
};


template< class ParticleClass, class CellClass, class ContainerClass>
const FReal FFmbKernelsBlockBlas<ParticleClass,CellClass,ContainerClass>::PiArrayInner[4] = {0, FMath::FPiDiv2, FMath::FPi, -FMath::FPiDiv2};


template< class ParticleClass, class CellClass, class ContainerClass>
const FReal FFmbKernelsBlockBlas<ParticleClass,CellClass,ContainerClass>::PiArrayOuter[4] = {0, -FMath::FPiDiv2, FMath::FPi, FMath::FPiDiv2};



#endif //FFMBKERNELSBLOCKBLAS_HPP


