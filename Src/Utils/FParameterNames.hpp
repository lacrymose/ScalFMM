// ===================================================================================
// Copyright ScalFmm 2011 INRIA, Olivier Coulaud, Berenger Bramas, Matthias Messner
// olivier.coulaud@inria.fr, berenger.bramas@inria.fr
// This software is a computer program whose purpose is to compute the FMM.
//
// This software is governed by the CeCILL-C and LGPL licenses and
// abiding by the rules of distribution of free software.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public and CeCILL-C Licenses for more details.
// "http://www.cecill.info".
// "http://www.gnu.org/licenses".
// ===================================================================================
#ifdef FPARAMETERNAMES_HPP
#error FParameterNames must be included only once by each main file!
#else
#define FPARAMETERNAMES_HPP

#include "Utils/FGlobal.hpp"

#include "Utils/FParameters.hpp"

#include <iostream>
#include <vector>
#include <chrono>
#include <unistd.h>

/**
 * This file contains some useful classes/functions to manage the parameters,
 * but it also contains all the parameters definitions!
 * A Scalfmm executable must define a static object to propose a parameter to the user.
 */

/** A parameter descriptor */
struct FParameterNames {
    std::vector<const char*> options;
    const char* description;
};

/** The namespace where we put all the definitions */
namespace FParameterDefinitions {

static const FParameterNames Help = {
    {"-help", "--help"} ,
     "To have print the options used by the application."
};

static const FParameterNames Compile = {
    {"-show-compile", "--show-compile", "--flags"} ,
     "To have the list of flags and lib linked to scalfmm."
};

static const FParameterNames DateHost = {
    {"-show-info", "--show-host", "--datehost"} ,
     "To have to print the current host and the execution date."
};

static const FParameterNames NbParticles = {
    {"-nb", "--number-of-particles", "-N"} ,
     "The number of particles if they are generated by the executable."
};

static const FParameterNames OctreeHeight = {
    {"-h", "--height", "-depth"} ,
     "The number of levels in the octree (at least 2 for the root and the leaves)."
};

static const FParameterNames OctreeSubHeight = {
    {"-sh", "--sub-height", "-subdepth"} ,
     "The number of allocated levels in the sub octree."
};

static const FParameterNames InputFile = {
    {"-f", "-fin", "--input-filename", "-filename"} ,
     "To give an input file."
};

static const FParameterNames InputFileOne = {
    {"-f1", "-fin1", "--file-one"} ,
     "To give the first input file."
};

static const FParameterNames InputFileTwow = {
    {"-f2", "-fin2", "--file-two"} ,
     "To give the second input file."
};

static const FParameterNames InputBinFormat = {
    {"-binin", "-bininput", "--binary-input" } ,
     "To input is in binary format."
};

static const FParameterNames OutputFile = {
    {"-fout", "--output-filename"} ,
     "To give the output filename."
};

static const FParameterNames OutputVisuFile = {
    {"-fvisuout"} ,
     "To give the output filename in visu format."
};
static const FParameterNames FormatVisuFile{
	        {"-visufmt","-visu-fmt"},
	        "To specify format for the visu file (vtk, vtp, cvs or cosmo). vtp is the default"
	    };


static const FParameterNames OutputBinFormat = {
    {"-binout", "-binoutput"} ,
     "To output in binary format."
};

static const FParameterNames NbThreads = {
    {"-t", "-nbthreads"} ,
     "To choose the number of threads."
};

static const FParameterNames SequentialFmm = {
    {"-sequential", "--sequential-fmm"} ,
     "No parallelization in the FMM algorithm."
};

static const FParameterNames TaskFmm = {
    {"-task", "--task-fmm"} ,
     "Task parallelization in the FMM algorithm."
};

static const FParameterNames SHDevelopment = {
    {"-devp", "-sh-p"} ,
     "The degree of development for the spherical harmonic kernel (P)."
};

static const FParameterNames EnabledVerbose = {
    {"-verbose", "--verbose"} ,
     "To have a high degree of verbosity."
};

static const FParameterNames PeriodicityNbLevels = {
    {"-per", "--periodic-degree"} ,
     "The number of level upper to the root to proceed."
};

static const FParameterNames PeriodicityDisabled = {
    {"-noper", "--no-periodicity"} ,
     "To disable the periodicity."
};

static const FParameterNames DeltaT = {
    {"-dt", "--delta-time"} ,
     "The time step between iterations."
};

static const FParameterNames RotationKernel = {
    {"-rotation", "--rotation-kernel"} ,
     "To use the rotation kernel (based on spherical harmonics)."
};

static const FParameterNames SphericalKernel = {
    {"-spherical", "--spherical-kernel"} ,
     "To use the spherical harmonics old kernel."
};

static const FParameterNames ChebyshevKernel = {
    {"-chebyshev", "--chebyshev-kernel"} ,
     "To use the Chebyshev kernel."
};

static const FParameterNames Epsilon = {
    {"-epsilon", "--epsilon"} ,
     "The epsilon needed for the application."
};

static const FParameterNames PhysicalValue = {
    {"-pv", "--physical-value"} ,
     "The physical value of the particles."
};

/** To print a list of parameters */
inline void PrintUsedOptions(const std::vector<FParameterNames>& options){
    std::cout << ">> Here is the list of the parameters you can pass to this application :\n";
    for(const FParameterNames& option : options ){
        std::cout << ">> Descriptions : " << option.description << "\n";
        std::cout << "\t Params : ";
        for(const char* name : option.options ){
            std::cout << name << ", ";
        }
        std::cout << "\n";
    }
}

inline void PrintFlags(){
    std::cout << "This executable has been compiled with:\n";
    std::cout << "× Flags = " << SCALFMMCompileFlags << "\n";
    std::cout << "× Libs  = " << SCALFMMCompileLibs << "\n";
    std::cout.flush();
}

inline void PrintDateHost(){
    std::cout << "This execution is on:\n";
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "× Date = " << std::ctime(&now) << "\n";
    char hostname[1024];
    gethostname(hostname, 1024);
    std::cout << "× Host  = " << hostname << "\n";
    std::cout.flush();
}

}// End of namespace

/** This should be include at the beginin of all main file
 *  @code FHelpAndExit(argc, argv, FParameterDefinitions::NbParticles, FParameterNames OctreeSubHeight );
 */
#define FHelpAndExit(argc, argv, ...) \
    if(FParameters::existParameter(argc, argv, FParameterDefinitions::Compile.options)) {\
        FParameterDefinitions::PrintFlags();\
    } \
    if(FParameters::existParameter(argc, argv, FParameterDefinitions::DateHost.options)) {\
        FParameterDefinitions::PrintDateHost();\
    } \
    if(FParameters::existParameter(argc, argv, FParameterDefinitions::Help.options)) {\
        const std::vector<FParameterNames> optionsvec = {FParameterDefinitions::Compile, FParameterDefinitions::DateHost, __VA_ARGS__};\
        FParameterDefinitions::PrintUsedOptions(optionsvec);\
        return 0;\
    } \

/** This should be include at the beginin of all main file
 *  @code FHelpDescribeAndExit(argc, argv,
 *  @code       "This executable is doing this and this.",
 *  @code       FParameterDefinitions::NbParticles, FParameterNames OctreeSubHeight );
 */
#define FHelpDescribeAndExit(argc, argv, description, ...) \
    if(FParameters::existParameter(argc, argv, FParameterDefinitions::Compile.options)) {\
        FParameterDefinitions::PrintFlags();\
    } \
    if(FParameters::existParameter(argc, argv, FParameterDefinitions::DateHost.options)) {\
        FParameterDefinitions::PrintDateHost();\
    } \
    if(FParameters::existParameter(argc, argv, FParameterDefinitions::Help.options)) {\
        std::cout << argv[0] << " : " << description << "\n"; \
        const std::vector<FParameterNames> optionsvec = {FParameterDefinitions::Compile, FParameterDefinitions::DateHost, __VA_ARGS__};\
        FParameterDefinitions::PrintUsedOptions(optionsvec);\
        return 0;\
    } \
    {\
        std::cout << "[ScalFMM] To have the help for this executable pass: "; \
        for(auto pr: FParameterDefinitions::Help.options) std::cout << pr << ", "; \
        std::cout << "\n";\
    }


#endif // FPARAMETERNAMES_HPP
