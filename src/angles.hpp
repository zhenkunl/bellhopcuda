#pragma once
#include "common.hpp"

struct AnglesStructure {
    int32_t Nalpha = 0, Nbeta = 1, iSingle_alpha = -1, iSingle_beta = -1;
    real Dalpha, Dbeta;
    real *alpha, *beta;
};

inline void ReadRayElevationAngles(real freq, real Depth,
    const char (&TopOpt)[6], const char (&RunType)[6],
    LDIFile &ENVFile, std::ostream &PRTFile,
    AnglesStructure *Angles, Position *Pos)
{
    constexpr real c0 = RC(1500.0);
    
    if(TopOpt[5] == 'I'){
        // option to trace a single beam
        ENVFile.List(); ENVFile.Read(Angles->Nalpha); ENVFile.Read(Angles->iSingle_alpha);
    }else{
        ENVFile.List(); ENVFile.Read(Angles->Nalpha);
    }
    
    if(Angles->Nalpha == 0){ // automatically estimate Nalpha to use
        if(RunType[0] == 'R'){
            Angles->Nalpha = 50; // For a ray trace plot, we don't want too many rays ...
        }else{
            // you're letting ME choose? OK: ideas based on an isospeed ocean
            // limit based on phase of adjacent beams at maximum range
            Angles->Nalpha = STD::max((int)(RC(0.3) * Pos->Rr[Pos->NRr-1] * freq / c0), 300);
            
            // limit based on having beams that are thin with respect to the water depth
            // assumes also a full 360 degree angular spread of rays
            // Should check which Depth is used here, in case where there is a variable bathymetry
            real d_theta_recommended = STD::atan(Depth / (RC(10.0) * Pos->Rr[Pos->NRr-1]));
            Angles->Nalpha = STD::max((int)(M_PI / d_theta_recommended), Angles->Nalpha);
        }
    }
    
    Angles->alpha = allocate<real>(STD::max(3, Angles->Nalpha));
    
    if(Angles->Nalpha > 2) Angles->alpha[2] = RC(-999.9);
    ENVFile.List(); ENVFile.Read(Angles->alpha, Angles->Nalpha);
    
    SubTab(Angles->alpha, Angles->Nalpha);
    Sort(  Angles->alpha, Angles->Nalpha);
    
    CheckFix360Sweep(Angles->alpha, Angles->Nalpha);
    
    PRTFile << "__________________________________________________________________________\n\n";
    PRTFile << "Number of beams in elevation   = " << Angles->Nalpha << "\n";
    if(Angles->iSingle_alpha > 0) PRTFile << "Trace only beam number " << Angles->iSingle_alpha << "\n";
    PRTFile << "Beam take-off angles (degrees)\n";
    
    EchoVector(Angles->alpha, Angles->Nalpha);
    
    if(Angles->Nalpha > 1 && Angles->alpha[Angles->Nalpha-1] == Angles->alpha[0]){
        std::cout << "ReadRayElevationAngles: First and last beam take-off angle are identical\n";
        std::abort();
    }
    
    if(TopOpt[5] == 'I'){
        if(Angles->iSingleAlpha < 1 || Angles->iSingle_alpha > Angles->Nalpha){
            std::cout << "'ReadRayElevationAngles: Selected beam, iSingl not in [1, Angles->Nalpha]\n";
            std::abort();
        }
    }
}