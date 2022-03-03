#include "common.hpp"
#include "readenv.hpp"
#include "boundary.hpp"
#include "refcoef.hpp"
#include "ssp.hpp"
#include "beams.hpp"
#include "run.hpp"

#ifdef BUILD_CUDA
void setupGPU();
#else

constexpr bool Init_Inline = false;

BHC_API void setup(std::string FileRoot, std::ostream &PRTFile, bhcParams &params,
    bhcOutputs &outputs)
{
    #ifdef BUILD_CUDA
    setupGPU();
    #else

    // Allocate main structs
    params.Bdry = allocate<BdryType>();
    params.bdinfo = allocate<BdryInfo>();
    params.refl = allocate<ReflectionInfo>();
    params.ssp = allocate<SSPStructure>();
    params.atten = allocate<AttenInfo>();
    params.Pos = allocate<Position>();
    params.Angles = allocate<AnglesStructure>();
    params.freqinfo = allocate<FreqInfo>();
    params.Beam = allocate<BeamStructure>();
    params.beaminfo = allocate<BeamInfo>();
    outputs.eigen = allocate<EigenInfo>();
    outputs.arrinfo = allocate<ArrInfo>();
    HSInfo RecycledHS; //Values only initialized once--reused from top to ssp, and ssp to bot
    
    // Set pointers to null because BELLHOP checks if some of them are allocated
    // before allocating them
    params.bdinfo->Top = nullptr;
    params.bdinfo->Bot = nullptr;
    params.refl->RBot = nullptr;
    params.refl->RTop = nullptr;
    params.ssp->cMat = nullptr;
    params.ssp->czMat = nullptr;
    params.ssp->cMat3 = nullptr;
    params.ssp->czMat3 = nullptr;
    params.Pos->iSz = nullptr;
    params.Pos->iRz = nullptr;
    params.Pos->Sx = nullptr;
    params.Pos->Sy = nullptr;
    params.Pos->Sz = nullptr;
    params.Pos->Rr = nullptr;
    params.Pos->Rz = nullptr;
    params.Pos->ws = nullptr;
    params.Pos->wr = nullptr;
    params.Pos->theta = nullptr;
    params.Angles->alpha = nullptr;
    params.Angles->beta = nullptr;
    params.freqinfo->freqVec = nullptr;
    params.beaminfo->SrcBmPat = nullptr;
    outputs.eigen->hits = nullptr;
    outputs.arrinfo->Arr = nullptr;
    outputs.arrinfo->NArr = nullptr;
    outputs.ray2D = nullptr;
    outputs.uAllSources = nullptr;
    
    // Fill in default / "constructor" data
    params.fT = RL(1.0e20);
    //Bdry: none
    params.bdinfo->NATIPts = 2;
    params.bdinfo->NBTYPts = 2;
    memcpy(params.bdinfo->atiType, "LS", 2);
    memcpy(params.bdinfo->btyType, "LS", 2);
    //params.refl: none
    //params.ssp: none
    params.atten->t = FL(20.0);
    params.atten->Salinity = FL(35.0);
    params.atten->pH = FL(8.0);
    params.atten->z_bar = FL(0.0);
    params.Pos->NSx = 1;
    params.Pos->NSy = 1;
    params.Angles->Nalpha = 0;
    params.Angles->Nbeta = 1;
    //LP: not a typo; this is an index, one less than the start of the array,
    //which in Fortran (and in the env file!) is 0. This gets converted to 0-
    //indexed when it is used.
    params.Angles->iSingle_alpha = 0;
    params.Angles->iSingle_beta = 0;
    //params.freqinfo: none
    params.Beam->epsMultiplier = FL(1.0);
    memcpy(params.Beam->Type, "G S ", 4);
    //params.beaminfo: none
    outputs.eigen->neigen = 0;
    outputs.eigen->memsize = 0;
    outputs.arrinfo->MaxNArr = 1;
    RecycledHS.alphaR = FL(1500.0);
    RecycledHS.betaR = FL(0.0);
    RecycledHS.alphaI = FL(0.0);
    RecycledHS.betaI = FL(0.0);
    RecycledHS.rho = FL(1.0);
    
    if(Init_Inline){
        // NPts, Sigma not used by BELLHOP
        params.Title = PROGRAMNAME "- Calibration case with envfil passed as parameters\n";
        params.freqinfo->freq0 = FL(250.0);
        // NMedia variable is not used by BELLHOP
        
        // *** Boundary information (type of boundary condition and, if a halfspace, then halfspace info)
        
        memcpy(params.ssp->AttenUnit, "W", 2); //LP: not a typo--one character string assigned to two
        params.Bdry->Top.hs.bc    = 'V';
        params.Bdry->Top.hs.Depth = FL(0.0);
        params.Bdry->Bot.hs.Depth = FL(100.0);
        memcpy(params.Bdry->Bot.hs.Opt, "A_", 2);
        params.Bdry->Bot.hs.bc    = 'A';
        params.Bdry->Bot.hs.cP    = crci(RL(1.0e20), RL(1590.0), RL(0.5),
            params.freqinfo->freq0, params.freqinfo->freq0, params.ssp->AttenUnit,
            betaPowerLaw, params.fT, params.atten, PRTFile); // compressional wave speed
        params.Bdry->Bot.hs.cS    = crci(RL(1.0e20), RL(0.0)   , RL(0.0),
            params.freqinfo->freq0, params.freqinfo->freq0, params.ssp->AttenUnit,
            betaPowerLaw, params.fT, params.atten, PRTFile); // shear         wave speed
        params.Bdry->Bot.hs.rho   = FL(1.2);
        
        // *** sound speed in the water column ***
        
        params.ssp->Type = 'C'; // interpolation method for SSP
        params.ssp->NPts = 2;   // number of SSP points
        params.ssp->z[0]  = FL(0.0);    params.ssp->z[1]  = FL(100.0);
        params.ssp->c[0]  = FL(1500.0); params.ssp->c[1]  = FL(1500.0);
        params.ssp->cz[0] = FL(0.0);    params.ssp->cz[1] = FL(0.0); // user should really not have to supply this ...
        
        // *** source and receiver positions ***
        
        params.Pos->NSz = 1;
        params.Pos->NRz = 100;
        params.Pos->NRr = 500;
        
        params.Pos->Sz = allocate<float>(params.Pos->NSz);
            params.Pos->ws = allocate<float>(params.Pos->NSz);
                params.Pos->iSz = allocate<int32_t>(params.Pos->NSz);
        params.Pos->Rz = allocate<float>(params.Pos->NRz);
            params.Pos->wr = allocate<float>(params.Pos->NRz);
                params.Pos->iRz = allocate<int32_t>(params.Pos->NRz);
        params.Pos->Rr = allocate<float>(params.Pos->NRr);
        
        memcpy(params.Beam->RunType, "C      ", 7);
        memcpy(params.Beam->Type, "G   ", 4);
        params.Beam->deltas  = FL(0.0);
        params.Beam->Box.z   = FL(101.0);
        params.Beam->Box.r   = FL(5100.0); // meters
        
        params.Angles->Nalpha = 1789;
        //params.Angles->alpha = {-80, -70, -60, -50, -40, -30, -20, -10, 0, 10, 20, 30, 40, 50, 60, 70, 80}; // -89 89
        for(int32_t jj=0; jj<params.Angles->Nalpha; ++jj){
            params.Angles->alpha[jj] = (FL(180.0) / params.Angles->Nalpha) 
                * (real)jj - FL(90.0);
        }
        
        // *** altimetry ***
        
        params.bdinfo->Top = allocate<BdryPtFull>(2);
        params.bdinfo->Top[0].x = vec2(-BdryInfinity, RL(0.0));
        params.bdinfo->Top[1].x = vec2( BdryInfinity, RL(0.0));
        
        ComputeBdryTangentNormal(params.bdinfo->Top, true, params.bdinfo);
        
        // *** bathymetry ***
        
        params.bdinfo->Bot = allocate<BdryPtFull>(2);
        params.bdinfo->Bot[0].x = vec2(-BdryInfinity, RL(5000.0));
        params.bdinfo->Bot[1].x = vec2( BdryInfinity, RL(5000.0));
        
        ComputeBdryTangentNormal(params.bdinfo->Bot, false, params.bdinfo);
        
        params.refl->RBot = allocate<ReflectionCoef>(1);
        params.refl->RTop = allocate<ReflectionCoef>(1);
        params.refl->NBotPts = params.refl->NTopPts = 1;
        
        // *** Source Beam Pattern ***
        params.beaminfo->NSBPPts = 2;
        params.beaminfo->SrcBmPat = allocate<real>(2*2);
        params.beaminfo->SrcBmPat[0*2+0] = FL(-180.0); params.beaminfo->SrcBmPat[0*2+1] = FL(0.0);
        params.beaminfo->SrcBmPat[1*2+0] = FL( 180.0); params.beaminfo->SrcBmPat[1*2+1] = FL(0.0);
        for(int32_t i=0; i<2; ++i) params.beaminfo->SrcBmPat[i*2+1] = 
            STD::pow(FL(10.0), params.beaminfo->SrcBmPat[i*2+1] / FL(20.0)); // convert dB to linear scale !!!
    }else{
        ReadEnvironment(FileRoot, PRTFile, params.Title, params.fT, params.Bdry,
            params.ssp, params.atten, params.Pos, params.Angles, params.freqinfo,
            params.Beam, params.RecycledHS);
        ReadATI(FileRoot, params.Bdry->Top.hs.Opt[4], params.Bdry->Top.hs.Depth,
            PRTFile, params.bdinfo); // AlTImetry
        ReadBTY(FileRoot, params.Bdry->Bot.hs.Opt[1], params.Bdry->Bot.hs.Depth,
            PRTFile, params.bdinfo); // BaThYmetry
        ReadReflectionCoefficient(FileRoot, 
            params.Bdry->Bot.hs.Opt[0], params.Bdry->Top.hs.Opt[1], PRTFile, params.refl); // (top and bottom)
        params.beaminfo->SBPFlag = params.Beam->RunType[2];
        ReadPat(FileRoot, PRTFile, params.beaminfo); // Source Beam Pattern
        // dummy bearing angles
        params.Pos->Ntheta = 1;
        params.Pos->theta = allocate<float>(params.Pos->Ntheta);
        params.Pos->theta[0] = FL(0.0);
    }
    
    // LP: Moved from WriteHeader
    // receiver bearing angles
    if(params.Pos->theta == nullptr){
        params.Pos->theta = allocate<float>(1);
        params.Pos->theta[0] = FL(0.0); // dummy bearing angle
        params.Pos->Ntheta = 1;
    }
    // source x-coordinates
    if(params.Pos->Sx == nullptr){
        params.Pos->Sx = allocate<float>(1);
        params.Pos->Sx[0] = FL(0.0); // dummy x-coordinate
        params.Pos->NSx = 1;
    }
    // source y-coordinates
    if(params.Pos->Sy == nullptr){
        params.Pos->Sy = allocate<float>(1);
        params.Pos->Sy[0] = FL(0.0); // dummy y-coordinate
        params.Pos->NSy = 1;
    }
    
    if(params.Beam->deltas == FL(0.0)){
         // Automatic step size selection
        params.Beam->deltas = (params.Bdry->Bot.hs.Depth - params.Bdry->Top.hs.Depth) / FL(10.0);
        PRTFile << "\n Step length,       deltas = " << params.Beam->deltas << " m (automatically selected)\n";
    }
    
    for(int32_t i=0; i<params.Angles->Nalpha; ++i)
        params.Angles->alpha[i] *= DegRad; // convert to radians
    params.Angles->Dalpha = FL(0.0);
    if(params.Angles->Nalpha != 1)
        params.Angles->Dalpha = (params.Angles->alpha[params.Angles->Nalpha-1] 
            - params.Angles->alpha[0]) / (params.Angles->Nalpha-1);
    
    // convert range-dependent geoacoustic parameters from user to program units
    if(params.bdinfo->atiType[1] == 'L'){
        for(int32_t iSeg = 0; iSeg < params.bdinfo->NATIPts; ++iSeg){
             // compressional wave speed
            params.bdinfo->Top[iSeg].hs.cP = crci(RL(1.0e20),
                params.bdinfo->Top[iSeg].hs.alphaR,
                params.bdinfo->Top[iSeg].hs.alphaI,
                params.freqinfo->freq0, params.freqinfo->freq0,
                {'W', ' '}, betaPowerLaw, params.fT, params.atten, PRTFile);
             // shear         wave speed
            params.bdinfo->Top[iSeg].hs.cS = crci(RL(1.0e20),
                params.bdinfo->Top[iSeg].hs.betaR,
                params.bdinfo->Top[iSeg].hs.betaI, 
                params.reqinfo->freq0, fparams.reqinfo->freq0,
                {'W', ' '}, betaPowerLaw, params.fT, params.atten, PRTFile);
        }
    }
    
    if(params.bdinfo->btyType[1] == 'L'){
        for(int32_t iSeg = 0; iSeg < params.bdinfo->NBTYPts; ++iSeg){
             // compressional wave speed
            params.bdinfo->Bot[iSeg].hs.cP = crci(RL(1.0e20),
                params.bdinfo->Bot[iSeg].hs.alphaR,
                params.bdinfo->Bot[iSeg].hs.alphaI,
                params.freqinfo->freq0, params.freqinfo->freq0,
                {'W', ' '}, betaPowerLaw, params.fT, params.atten, PRTFile);
             // shear         wave speed
            params.bdinfo->Bot[iSeg].hs.cS = crci(RL(1.0e20),
                params.bdinfo->Bot[iSeg].hs.betaR,
                params.bdinfo->Bot[iSeg].hs.betaI, 
                params.reqinfo->freq0, fparams.reqinfo->freq0,
                {'W', ' '}, betaPowerLaw, params.fT, params.atten, PRTFile);
        }
    }
    
    PRTFile << "\n";
}
