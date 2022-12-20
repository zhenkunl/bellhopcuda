/*
bellhopcxx / bellhopcuda - C++/CUDA port of BELLHOP underwater acoustics simulator
Copyright (C) 2021-2022 The Regents of the University of California
c/o Jules Jaffe team at SIO / UCSD, jjaffe@ucsd.edu
Based on BELLHOP, which is Copyright (C) 1983-2020 Michael B. Porter

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#ifndef _BHC_INCLUDED_
#error "This file must be included via #include <bhc/bhc.hpp>!"
#endif

#include <type_traits>

namespace bhc {

/*
O3D: ocean (SSP, boundaries, etc.) is 3D.
R3D: rays (beams) are 3D. In 2D-3D mode, S3D == true && R3D == false.
X3D: generic 3D boolean value for templated types.
*/

template<bool X3D> struct TmplVec23 {};
template<> struct TmplVec23<false> {
    typedef vec2 type;
};
template<> struct TmplVec23<true> {
    typedef vec3 type;
};
template<bool X3D> using VEC23 = typename TmplVec23<X3D>::type;

template<bool X3D> struct TmplInt12 {};
template<> struct TmplInt12<false> {
    typedef int32_t type;
};
template<> struct TmplInt12<true> {
    typedef int2 type;
};
template<bool X3D> using IORI2 = typename TmplInt12<X3D>::type;

template<bool X3D> struct TmplVec2Mat2 {};
template<> struct TmplVec2Mat2<false> {
    typedef vec2 type;
};
template<> struct TmplVec2Mat2<true> {
    typedef mat2x2 type;
};
template<bool X3D> using V2M2 = typename TmplVec2Mat2<X3D>::type;

////////////////////////////////////////////////////////////////////////////////
// SSP / Boundary
////////////////////////////////////////////////////////////////////////////////

constexpr int32_t MaxN   = 100000;
constexpr int32_t MaxSSP = MaxN + 1;

struct rxyz_vector {
    real *r, *x, *y, *z;
};

struct SSPStructure {
    int32_t NPts, Nr, Nx, Ny, Nz;
    real z[MaxSSP], rho[MaxSSP];
    cpx c[MaxSSP], cz[MaxSSP], n2[MaxSSP], n2z[MaxSSP], cSpline[4][MaxSSP];
    cpx cCoef[4][MaxSSP], CSWork[4][MaxSSP]; // for PCHIP coefs.
    real *cMat, *czMat; // LP: No need for separate cMat3 / czMat3 as we don't have to
                        // specify the dimension here.
    rxyz_vector Seg;
    char Type;
    char AttenUnit[2];
    real alphaR[MaxSSP], alphaI[MaxSSP];
    bool dirty; // reset and update derived params
};

template<bool R3D> struct SSPOutputsExtras {};
template<> struct SSPOutputsExtras<false> {
    real crr, crz;
};
template<> struct SSPOutputsExtras<true> {
    real cxx, cyy, cxy, cxz, cyz;
};
template<bool R3D> struct SSPOutputs : public SSPOutputsExtras<R3D> {
    cpx ccpx;
    VEC23<R3D> gradc;
    real rho, czz;
};

struct SSPSegState {
    int32_t r, x, y, z;
};

struct HSInfo {
    real alphaR, betaR, alphaI, betaI; // compressional and shear wave speeds/attenuations
                                       // in user units
    cpx cP, cS;                        // P-wave, S-wave speeds
    real rho, Depth;                   // density, depth
    char bc;                           // Boundary condition type
    char Opt[6];
};

struct BdryPtSmall {
    HSInfo hs;
};

/**
 * LP: There are three boundary structures. This one actually existed in the
 * FORTRAN, and is called Bdry. This holds the boundary properties for the
 * current segment.
 */
struct BdryType {
    BdryPtSmall Top, Bot;
};

template<bool O3D> struct ReflCurvature {};
template<> struct ReflCurvature<false> {
    real kappa; // curvature of a segment
};
template<> struct ReflCurvature<true> {
    real z_xx, z_xy, z_yy, kappa_xx, kappa_xy, kappa_yy;
};

template<bool O3D> struct BdryPtFullExtras {};
template<> struct BdryPtFullExtras<false> {
    vec2 Nodet;        // tangent at the node, if the curvilinear option is used
    real Dx, Dxx, Dss; // first, second derivatives wrt depth; s is along tangent
    HSInfo hs;
};
template<> struct BdryPtFullExtras<true> {
    vec3 n1, n2; // (outward-pointing) normals for each of the triangles in a pair, n is
                 // selected from those
    vec3 Noden_unscaled;
    real phi_xx, phi_xy, phi_yy;
};
template<bool O3D> struct BdryPtFull : public BdryPtFullExtras<O3D>,
                                       public ReflCurvature<O3D> {
    VEC23<O3D> x; // 2D: coordinate for a segment / 3D: coordinate of boundary
    VEC23<O3D> t; // 2D: tangent for a segment / 3D: tangent for a facet
    VEC23<O3D> n; // 2D: outward normal for a segment / 3D: normal for a facet (outward
                  // pointing)
    VEC23<O3D> Noden; // normal at the node, if the curvilinear option is used
    real Len; // 2D: length of a segment / 3D: length of tangent (temporary variable to
              // normalize tangent)
};

template<bool O3D> struct BdryInfoTopBot {
    IORI2<O3D> NPts;
    char type[2];        // In 3D, only first char is used
    BdryPtFull<O3D> *bd; // 2D: 1D array / 3D: 2D array
};
/**
 * LP: There are three boundary structures. This one represents static/global
 * data in the FORTRAN which was not within any structure, and is called bdinfo.
 */
template<bool O3D> struct BdryInfo {
    BdryInfoTopBot<O3D> top, bot;
};

struct BdryLimits {
    real min, max;
};
struct BdryLimits2 {
    BdryLimits x, y;
};
template<bool O3D> struct TmplBdryLimits12 {};
template<> struct TmplBdryLimits12<false> {
    typedef BdryLimits type;
};
template<> struct TmplBdryLimits12<true> {
    typedef BdryLimits2 type;
};
template<bool O3D> struct BdryStateTopBotExtras {};
template<> struct BdryStateTopBotExtras<true> {
    bool tridiag_pos;
};
template<bool O3D> struct BdryStateTopBot : public BdryStateTopBotExtras<O3D> {
    IORI2<O3D> Iseg; // indices that point to the current active segment
    typename TmplBdryLimits12<O3D>::type lSeg; // LP: limits of current segment
    VEC23<O3D> x, n; // only explicitly written in 3D, but effectively present in 2D
    VEC23<O3D> xmid; // coordinates of center of active rectangle (3D) / segment (2D)
    // because corners may be at big number and mess up floating point precision
};
/**
 * LP: There are three boundary structures. This one holds variables describing
 * where the ray currently is in terms of boundary segments, and is called bds.
 */
template<bool O3D> struct BdryState {
    BdryStateTopBot<O3D> top, bot;
};

/**
 * LP: In 2D-3D mode, describes the position of the 2D ray space relative to the
 * 3D ocean. Unused (empty struct) in 2D and full 3D mode.
 */
template<bool O3D, bool R3D> struct Origin {};
template<> struct Origin<true, false> {
    vec3 xs;
    vec2 tradial;
};

////////////////////////////////////////////////////////////////////////////////
// Reflections
////////////////////////////////////////////////////////////////////////////////

struct ReflectionCoef {
    real theta, r, phi;
};

struct ReflectionInfoTopBot {
    int32_t NPts;
    ReflectionCoef *r;
};
struct ReflectionInfo {
    ReflectionInfoTopBot bot, top;
};

////////////////////////////////////////////////////////////////////////////////
// Attenuation (volume absorption / scattering)
////////////////////////////////////////////////////////////////////////////////

struct bioStructure {
    real z1, z2, f0, q, a0;
};

constexpr int32_t MaxBioLayers = 200;

struct AttenInfo {
    int32_t NBioLayers;
    bioStructure bio[MaxBioLayers];
    real t, Salinity, pH, z_bar, fg; // Francois-Garrison volume attenuation; temperature,
                                     // salinity, ...
};

////////////////////////////////////////////////////////////////////////////////
// Source / receiver positions
////////////////////////////////////////////////////////////////////////////////

struct Position {
    int32_t NSx, NSy, NSz, NRz, NRr, Ntheta; // number of x, y, z, r, theta coordinates
    int32_t NRz_per_range;
    real Delta_r, Delta_theta;
    int32_t *iSz, *iRz;
    // LP: These are really floats, not reals.
    float *Sx, *Sy, *Sz;      // Source x, y, z coordinates
    float *Rr, *Rz, *ws, *wr; // Receiver r, z coordinates and weights for interpolation
    float *theta;             // Receiver bearings
    vec2 *t_rcvr;             // Receiver directions (cos(theta), sin(theta))
};

////////////////////////////////////////////////////////////////////////////////
// Source angles
////////////////////////////////////////////////////////////////////////////////

struct AngleInfo {
    int32_t n, iSingle;
    real d; // angular spacing between beams
    real *angles;
};

struct AnglesStructure {
    AngleInfo alpha; // LP: elevation angles
    AngleInfo beta;  // LP: azimuth angles
};

////////////////////////////////////////////////////////////////////////////////
// Source frequencies
////////////////////////////////////////////////////////////////////////////////

struct FreqInfo {
    real freq0;    // Nominal or carrier frequency
    int32_t Nfreq; // number of frequencies
    real *freqVec; // frequency vector for braoaband runs
};

////////////////////////////////////////////////////////////////////////////////
// Beams
////////////////////////////////////////////////////////////////////////////////

/**
 * LP: Like boundaries, there are two beam structures. This one is (mostly)
 * in the FORTRAN, and is called Beam.
 */
template<bool O3D> struct BeamStructure {
    // LP: NSteps moved out of this struct as it's a property of a single beam.
    int32_t NBeams, Nimage, iBeamWindow;
    real deltas, epsMultiplier, rLoop;
    char Component;
    char Type[4];
    char RunType[7];
    VEC23<O3D> Box;
};

/**
 * LP: TODO: rename to something with SBP, this is not the same kind of beam
 * as BeamStructure.
 */
struct BeamInfo {
    int32_t NSBPPts;
    real *SrcBmPat;
    char SBPFlag;
};

////////////////////////////////////////////////////////////////////////////////
// Eigenrays
////////////////////////////////////////////////////////////////////////////////

struct EigenHit {
    // LP: Number of steps until the ray hit the receiver
    int32_t is;
    // LP: Receiver this hit pertains to
    int32_t itheta, ir, iz;
    // LP: Identifying info to re-trace this ray
    int32_t isx, isy, isz, ialpha, ibeta;
};

struct EigenInfo {
    uint32_t neigen;
    uint32_t memsize;
    EigenHit *hits;
};

////////////////////////////////////////////////////////////////////////////////
// Arrivals
////////////////////////////////////////////////////////////////////////////////

struct Arrival {
    int32_t NTopBnc, NBotBnc;
    float SrcDeclAngle, SrcAzimAngle, RcvrDeclAngle, RcvrAzimAngle, a, Phase;
    cpxf delay;
};

/**
 * LP: Arrival setup and results.
 */
struct ArrInfo {
    Arrival *Arr;
    int32_t *NArr;
    int32_t MaxNArr;
    size_t ArrMemSize;
    bool AllowMerging;
};

////////////////////////////////////////////////////////////////////////////////
// Rays/beams
////////////////////////////////////////////////////////////////////////////////

template<bool R3D> struct rayPtExtras {};
template<> struct rayPtExtras<false> {
    vec2 p, q;
};
template<> struct rayPtExtras<true> {
    // LP: p and q don't actually affect the ray trajectory or results; they are
    // used to update the corresponding variables for the next step, but that's
    // never actually used for anything else. There is commented out code which
    // uses them, as well as additional commented-out variables in this struct,
    // for 3D Cerveny beams.
    mat2x2 p, q; // LP: The ROWS of p are p_tilde and p_hat; same for q.
    // real DetQ; LP: Precomputed at the start of Influence functions, only used
    // for detecting phase inversions. Changed to compute when needed.
    real phi;
};
template<bool R3D> struct rayPt : public rayPtExtras<R3D> {
    int32_t NumTopBnc, NumBotBnc;
    /// ray coordinate, (r,z)
    VEC23<R3D> x;
    /// scaled tangent to the ray (previously (rho, zeta))
    VEC23<R3D> t;
    /// c * t would be the unit tangent
    real c;
    real Amp, Phase;
    cpx tau;
};

template<bool R3D> struct StepPartials {};
template<> struct StepPartials<false> {
    real cnn_csq;
};
template<> struct StepPartials<true> {
    real cnn, cmn, cmm;
};

template<bool O3D, bool R3D> struct RayResult {
    rayPt<R3D> *ray;
    Origin<O3D, R3D> org;
    real SrcDeclAngle;
    int32_t Nsteps;
};

template<bool O3D, bool R3D> struct RayInfo {
    rayPt<R3D> *raymem;
    uint32_t NPoints;
    uint32_t MaxPoints;
    RayResult<O3D, R3D> *results;
    int32_t NRays;
};

struct RayInitInfo {
    int32_t isx, isy, isz, ialpha, ibeta;
    real alpha, beta;
    real SrcDeclAngle, SrcAzimAngle;
};

////////////////////////////////////////////////////////////////////////////////
// Influence / transmission loss
////////////////////////////////////////////////////////////////////////////////

template<bool R3D> struct InfluenceRayInfo {
    // LP: Constants.
    RayInitInfo init;
    real Dalpha, Dbeta;     // angular spacing
    real c0;                // LP: c at start of ray
    cpx epsilon1, epsilon2; // beam constant
    VEC23<R3D> xs;          // source
    real freq0, omega;
    real RadMax;
    real BeamWindow;
    int32_t iBeamWindow2;
    real Ratio1; // scale factor (point source vs. line source)
    real rcp_q0, rcp_qhat0;
    // LP: Variables carried over between iterations.
    real phase;
    real qOld; // LP: Det_QOld in 3D
    VEC23<R3D> x;
    VEC23<R3D> rayn1, rayn2; // LP: rayn1 was rn, zn in 2D
    bool lastValid;
    int32_t kmah;
    int32_t ir;
    cpx gamma;
};

////////////////////////////////////////////////////////////////////////////////
// Meta-structures
////////////////////////////////////////////////////////////////////////////////

template<bool O3D, bool R3D> struct bhcParams {
    /// Initialized to -1 meaning "one thread per logical core"; can set to 1
    /// or some other value.
    int32_t maxThreads;
    char Title[80]; // Size determined by WriteHeader for TL
    real fT;
    BdryType *Bdry;
    BdryInfo<O3D> *bdinfo;
    ReflectionInfo *refl;
    SSPStructure *ssp;
    AttenInfo *atten;
    Position *Pos;
    AnglesStructure *Angles;
    FreqInfo *freqinfo;
    BeamStructure<O3D> *Beam;
    BeamInfo *beaminfo;
    /// Pointer to internal data structure for program (non-marine-related) state.
    void *internal;
};

template<bool O3D, bool R3D> struct bhcOutputs {
    RayInfo<O3D, R3D> *rayinfo;
    cpxf *uAllSources;
    EigenInfo *eigen;
    ArrInfo *arrinfo;
};

} // namespace bhc
