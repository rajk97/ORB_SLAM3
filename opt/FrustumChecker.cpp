#include "FrustumChecker.h"
#include <tracy/Tracy.hpp>
#include <cmath>
#include <xmmintrin.h>  // SSE intrinsics for fast rsqrt

namespace frustum_opt {

FrustumChecker::FrustumChecker(
    const CameraMock* camera,
    const Eigen::Matrix3f& Rcw,
    const Eigen::Vector3f& tcw,
    float minX, float maxX,
    float minY, float maxY,
    float bf,
    float logScaleFactor,
    int nScaleLevels
)
    : camera_(camera)
    , Rcw_(Rcw)
    , tcw_(tcw)
    , minX_(minX), maxX_(maxX)
    , minY_(minY), maxY_(maxY)
    , bf_(bf)
    , logScaleFactor_(logScaleFactor)
    , nScaleLevels_(nScaleLevels)
    , fx_(camera->getFx())
    , fy_(camera->getFy())
    , cx_(camera->getCx())
    , cy_(camera->getCy())
{
    // Precompute inverse pose
    Rwc_ = Rcw_.transpose();
    Ow_ = -Rwc_ * tcw_;
}

// =============================================================================
// V0: Original implementation (baseline from ORB-SLAM3)
// =============================================================================
bool FrustumChecker::isInFrustum_v0(MapPointMock* pMP, float viewingCosLimit)
{
    ZoneScopedN("[V0] BASELINE");
    
    pMP->mbTrackInView = false;
    pMP->mTrackProjX = -1;
    pMP->mTrackProjY = -1;

    // 3D in absolute coordinates
    Eigen::Matrix<float, 3, 1> P = pMP->getWorldPos();

    // Transform to camera coordinates
    const Eigen::Matrix<float, 3, 1> Pc = Rcw_ * P + tcw_;
    const float Pc_dist = Pc.norm();

    // Check positive depth
    const float& PcZ = Pc(2);
    const float invz = 1.0f / PcZ;
    if (PcZ < 0.0f)
        return false;

    // Project to image
    const Eigen::Vector2f uv = camera_->project(Pc);

    // Image bounds check
    if (uv(0) < minX_ || uv(0) > maxX_)
        return false;
    if (uv(1) < minY_ || uv(1) > maxY_)
        return false;

    pMP->mTrackProjX = uv(0);
    pMP->mTrackProjY = uv(1);

    // Check distance is in the scale invariance region of the MapPoint
    const float maxDistance = pMP->getMaxDistanceInvariance();
    const float minDistance = pMP->getMinDistanceInvariance();
    const Eigen::Vector3f PO = P - Ow_;
    const float dist = PO.norm();

    if (dist < minDistance || dist > maxDistance)
        return false;

    // Check viewing angle
    Eigen::Vector3f Pn = pMP->getNormal();

    const float viewCos = PO.dot(Pn) / dist;

    if (viewCos < viewingCosLimit)
        return false;

    // Predict scale in the image
    const int nPredictedLevel = pMP->predictScale(dist, logScaleFactor_, nScaleLevels_);

    // Data used by the tracking
    pMP->mbTrackInView = true;
    pMP->mTrackProjX = uv(0);
    pMP->mTrackProjXR = uv(0) - bf_ * invz;
    pMP->mTrackDepth = Pc_dist;
    pMP->mTrackProjY = uv(1);
    pMP->mnTrackScaleLevel = nPredictedLevel;
    pMP->mTrackViewCos = viewCos;

    return true;
}

// =============================================================================
// V1: Optimization attempt 1
// Optimizations applied:
// - Reorder checks: depth check first (cheapest), then bounds, then distance
// - Inline camera projection to avoid virtual call
// - Avoid redundant Pc.norm() computation (defer until needed)
// - Defer tracking data writes until we know point passes all checks
// - Squared distance comparison (avoid sqrt on rejection)
// - Squared viewing angle comparison (avoid sqrt+div on rejection)
// =============================================================================
bool FrustumChecker::isInFrustum_v1(MapPointMock* pMP, float viewingCosLimit)
{
    ZoneScopedN("[V1] OPTIMIZED");

    // 3D in absolute coordinates
    const Eigen::Vector3f P = pMP->getWorldPos();

    // Transform to camera coordinates
    const Eigen::Vector3f Pc = Rcw_ * P + tcw_;
    
    // Check positive depth FIRST (cheapest check)
    const float PcZ = Pc(2);
    if (PcZ < 0.0f)
        return false;

    const float invz = 1.0f / PcZ;
    
    // Inline camera projection (avoid virtual call)
    const float u = fx_ * Pc(0) * invz + cx_;
    const float v = fy_ * Pc(1) * invz + cy_;
    
    // Image bounds check
    if (u < minX_ || u > maxX_ || v < minY_ || v > maxY_)
        return false;
    
    // Check distance using squared comparison (avoid sqrt)
    const Eigen::Vector3f PO = P - Ow_;
    const float distSq = PO.squaredNorm();
    const float maxDistance = pMP->getMaxDistanceInvariance();
    const float minDistance = pMP->getMinDistanceInvariance();
    const float minDistSq = minDistance * minDistance; 
    const float maxDistSq = maxDistance * maxDistance; 
    
    if (distSq < minDistSq || distSq > maxDistSq)
        return false;
    
    // Check viewing angle using squared comparison (avoid sqrt+div)
    const Eigen::Vector3f Pn = pMP->getNormal();
    const float dotProd = PO.dot(Pn);
    const float limitSq = viewingCosLimit * viewingCosLimit;
    
    if (dotProd < 0.0f || dotProd * dotProd < limitSq * distSq)
        return false; 
    
    // All checks passed - compute expensive stuff only on success path
    // Fast reciprocal sqrt: ~5 cycles vs ~20 for std::sqrt
    const float invDist = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(distSq)));
    const float dist = distSq * invDist;  // dist = distSq * (1/sqrt(distSq))
    const float viewCos = dotProd * invDist;  // viewCos = dot / dist = dot * invDist
    
    // Fast rsqrt for Pc.norm() too
    const float PcDistSq = Pc.squaredNorm();
    const float Pc_dist = PcDistSq * _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(PcDistSq)));
    const int nPredictedLevel = pMP->predictScale(dist, logScaleFactor_, nScaleLevels_);

    // Write tracking data only on success
    pMP->mbTrackInView = true;
    pMP->mTrackProjX = u;
    pMP->mTrackProjY = v;
    pMP->mTrackProjXR = u - bf_ * invz;
    pMP->mTrackDepth = Pc_dist;
    pMP->mnTrackScaleLevel = nPredictedLevel;
    pMP->mTrackViewCos = viewCos;

    return true;
}

// =============================================================================
// V2: Optimization attempt 2
// Additional optimizations:
// - Squared viewing angle check to avoid sqrt in common rejection case
// - Fused bounds checking with branchless logic
// - Prefetch MapPoint data
// - Manual loop unrolling of matrix-vector multiply
// =============================================================================
bool FrustumChecker::isInFrustum_v2(MapPointMock* pMP, float viewingCosLimit)
{
    ZoneScopedN("[V2] AGGRESSIVE - Manual+SqViewAngle");
    
    // Get world position
    const Eigen::Vector3f P = pMP->getWorldPos();

    // Manual matrix-vector multiply (compiler may optimize better with explicit code)
    const float Pc_x = Rcw_(0,0)*P(0) + Rcw_(0,1)*P(1) + Rcw_(0,2)*P(2) + tcw_(0);
    const float Pc_y = Rcw_(1,0)*P(0) + Rcw_(1,1)*P(1) + Rcw_(1,2)*P(2) + tcw_(1);
    const float Pc_z = Rcw_(2,0)*P(0) + Rcw_(2,1)*P(1) + Rcw_(2,2)*P(2) + tcw_(2);
    
    // Check positive depth FIRST
    if (Pc_z < 0.0f)
        return false;
    
    const float invz = 1.0f / Pc_z;

    // Inline projection
    const float u = fx_ * Pc_x * invz + cx_;
    const float v = fy_ * Pc_y * invz + cy_;

    // Branchless bounds check using bitwise OR 
    // (all conditions must be false for the point to be valid)
    const bool outOfBounds = (u < minX_) | (u > maxX_) | (v < minY_) | (v > maxY_);
    if (outOfBounds)
        return false;

    // Compute PO vector
    const float PO_x = P(0) - Ow_(0);
    const float PO_y = P(1) - Ow_(1);
    const float PO_z = P(2) - Ow_(2);
    const float distSq = PO_x*PO_x + PO_y*PO_y + PO_z*PO_z;
    
    // Check distance bounds using squared comparison
    const float maxDistance = pMP->getMaxDistanceInvariance();
    const float minDistance = pMP->getMinDistanceInvariance();
    const float maxDistSq = maxDistance * maxDistance;
    const float minDistSq = minDistance * minDistance;
    
    if (distSq < minDistSq || distSq > maxDistSq)
        return false;
    
    // Get normal and compute dot product
    const Eigen::Vector3f Pn = pMP->getNormal();
    const float dotProd = PO_x*Pn(0) + PO_y*Pn(1) + PO_z*Pn(2);
    
    // Squared viewing angle check (avoid sqrt when rejecting)
    // viewCos = dot(PO, Pn) / dist
    // viewCos < limit  =>  dot / dist < limit
    // If dot >= 0: dot < limit * dist  =>  dot^2 < limit^2 * dist^2
    // If dot < 0: always fails (viewing from behind)
    const float viewCosLimitSq = viewingCosLimit * viewingCosLimit;
    if (dotProd < 0.0f || dotProd * dotProd < viewCosLimitSq * distSq)
        return false;

    // All checks passed - now compute expensive stuff only once
    const float dist = std::sqrt(distSq);
    const float viewCos = dotProd / dist;
    const float Pc_dist = std::sqrt(Pc_x*Pc_x + Pc_y*Pc_y + Pc_z*Pc_z);
    const int nPredictedLevel = pMP->predictScale(dist, logScaleFactor_, nScaleLevels_);

    // Write tracking data
    pMP->mbTrackInView = true;
    pMP->mTrackProjX = u;
    pMP->mTrackProjY = v;
    pMP->mTrackProjXR = u - bf_ * invz;
    pMP->mTrackDepth = Pc_dist;
    pMP->mnTrackScaleLevel = nPredictedLevel;
    pMP->mTrackViewCos = viewCos;

    return true;
}

} // namespace frustum_opt
