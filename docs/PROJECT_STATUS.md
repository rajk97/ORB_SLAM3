# ORB-SLAM3 XR Sprint - Project Status

**Date:** November 24, 2025  
**Author:** Raj  
**Repository:** https://github.com/rajk97/ORB_SLAM3

---

## 🎯 Project Goal

Set up and optimize ORB-SLAM3's **Mono-Inertial VIO pipeline** for XR (AR/VR) applications. This configuration mirrors real XR headset hardware (Quest, ARKit) which use a single camera + IMU.

---

## 📍 Current State

| Component | Status |
|-----------|--------|
| ORB-SLAM3 Build | ✅ Working |
| Mono-Inertial VIO | ✅ Working |
| EuRoC V1_01_easy | ✅ 2804 poses, 302 KFs |
| GUI Visualization | ✅ Working |
| Git Fork | ✅ Pushed |

---

## 🔧 Dependencies & Learnings

### System
- **OS:** Ubuntu 22.04
- **Compiler:** GCC 11 (C++14 required)
- **Hardware:** Intel i9-14900KF, NVIDIA RTX 4090, 62GB RAM

### Dependencies (All Built Locally)

| Dependency | Version | Location | Notes |
|------------|---------|----------|-------|
| **Eigen** | 3.4.0 | `/home/raj/Documents/Projects/system_software/eigen/install` | Header-only, standard install |
| **Pangolin** | v0.8 | `/home/raj/Documents/Projects/system_software/pangolin/install` | **Requires C++14** (not C++11!) |
| **OpenCV** | 4.4.0 | `/home/raj/Documents/Projects/system_software/opencv/install` | Built with GTK for GUI support |

### Key Build Fixes Applied

1. **C++14 Requirement:** Pangolin v0.8 requires C++14. Changed `CMakeLists.txt`:
   ```cmake
   set(CMAKE_CXX_STANDARD 14)  # Was 11
   ```

2. **Deprecated API Fix:** `std::chrono::monotonic_clock` is deprecated. Changed to `steady_clock` in all example files.

---

## 📊 Dataset

### EuRoC MAV Dataset
- **Source:** https://projects.asl.ethz.ch/datasets/doku.php?id=kmavvisualinertialdatasets
- **Format:** ASL format with stereo cameras + IMU
- **Location:** `/home/raj/Documents/Projects/datasets/EuRoC/`

### Downloaded Sequences

| Sequence | Type | Status | Notes |
|----------|------|--------|-------|
| MH_01_easy | Machine Hall (drone) | ⚠️ VIO fails | Stationary start breaks IMU init |
| V1_01_easy | Vicon Room (handheld) | ✅ Works | Recommended for VIO testing |
| V1_02_medium | Vicon Room (handheld) | Downloaded | More challenging motion |

### Why V-series for VIO?
- **MH-series (drone):** Starts stationary on ground → insufficient motion for VIO initialization
- **V-series (handheld):** Continuous motion from start → VIO initializes successfully

---

## 🚀 Commands to Replicate

### Quick Copy-Paste Commands

#### WITH GUI (Visuals On)

```bash
cd /home/raj/Documents/Projects/xr_slam_sprint/ORB_SLAM3 && \
export LD_LIBRARY_PATH="./lib:./Thirdparty/DBoW2/lib:./Thirdparty/g2o/lib:/home/raj/Documents/Projects/system_software/opencv/install/lib:/home/raj/Documents/Projects/system_software/pangolin/install/lib" && \
export DISPLAY=:1 && \
./Examples/Monocular-Inertial/mono_inertial_euroc \
  ./Vocabulary/ORBvoc.txt \
  ./Examples/Monocular-Inertial/EuRoC.yaml \
  /home/raj/Documents/Projects/datasets/EuRoC/V101 \
  ./Examples/Monocular-Inertial/EuRoC_TimeStamps/V101.txt \
  dataset-V101_mono_inertial
```

#### WITHOUT GUI (Headless)

```bash
cd /home/raj/Documents/Projects/xr_slam_sprint/ORB_SLAM3 && \
export LD_LIBRARY_PATH="./lib:./Thirdparty/DBoW2/lib:./Thirdparty/g2o/lib:/home/raj/Documents/Projects/system_software/opencv/install/lib:/home/raj/Documents/Projects/system_software/pangolin/install/lib" && \
export DISPLAY=:99 && \
./Examples/Monocular-Inertial/mono_inertial_euroc \
  ./Vocabulary/ORBvoc.txt \
  ./Examples/Monocular-Inertial/EuRoC.yaml \
  /home/raj/Documents/Projects/datasets/EuRoC/V101 \
  ./Examples/Monocular-Inertial/EuRoC_TimeStamps/V101.txt \
  dataset-V101_mono_inertial
```

> **Note:** For headless mode, ensure Xvfb is running: `Xvfb :99 -screen 0 1024x768x24 &`

---

### Other Datasets

#### V1_02_medium (WITH GUI)
```bash
cd /home/raj/Documents/Projects/xr_slam_sprint/ORB_SLAM3 && \
export LD_LIBRARY_PATH="./lib:./Thirdparty/DBoW2/lib:./Thirdparty/g2o/lib:/home/raj/Documents/Projects/system_software/opencv/install/lib:/home/raj/Documents/Projects/system_software/pangolin/install/lib" && \
export DISPLAY=:1 && \
./Examples/Monocular-Inertial/mono_inertial_euroc \
  ./Vocabulary/ORBvoc.txt \
  ./Examples/Monocular-Inertial/EuRoC.yaml \
  /home/raj/Documents/Projects/datasets/EuRoC/V102 \
  ./Examples/Monocular-Inertial/EuRoC_TimeStamps/V102.txt \
  dataset-V102_mono_inertial
```

#### V1_02_medium (WITHOUT GUI)
```bash
cd /home/raj/Documents/Projects/xr_slam_sprint/ORB_SLAM3 && \
export LD_LIBRARY_PATH="./lib:./Thirdparty/DBoW2/lib:./Thirdparty/g2o/lib:/home/raj/Documents/Projects/system_software/opencv/install/lib:/home/raj/Documents/Projects/system_software/pangolin/install/lib" && \
export DISPLAY=:99 && \
./Examples/Monocular-Inertial/mono_inertial_euroc \
  ./Vocabulary/ORBvoc.txt \
  ./Examples/Monocular-Inertial/EuRoC.yaml \
  /home/raj/Documents/Projects/datasets/EuRoC/V102 \
  ./Examples/Monocular-Inertial/EuRoC_TimeStamps/V102.txt \
  dataset-V102_mono_inertial
```

---

### Expected Output

```
New Map created with ~400-500 points
start VIBA 1
end VIBA 1
start VIBA 2
end VIBA 2
Shutdown
Saving trajectory to f_dataset-V101_mono_inertial.txt ...
```

### Output Files
- `f_dataset-V101_mono_inertial.txt` - Full trajectory (TUM format)
- `kf_dataset-V101_mono_inertial.txt` - Keyframe trajectory

---

## 📁 Project Structure

```
/home/raj/Documents/Projects/
├── xr_slam_sprint/
│   └── ORB_SLAM3/              # Main project (forked)
│       ├── docs/               # Documentation
│       ├── Examples/           # Executable examples
│       ├── Vocabulary/         # ORB vocabulary (ORBvoc.txt)
│       └── lib/                # Built libraries
│
├── datasets/
│   └── EuRoC/
│       ├── V101/mav0/          # V1_01_easy
│       ├── V102/mav0/          # V1_02_medium
│       └── MH01/mav0/          # MH_01_easy
│
└── system_software/
    ├── eigen/install/
    ├── pangolin/install/
    └── opencv/install/
```

---

## 📈 Baseline Results

### Mono-Inertial on V1_01_easy

| Metric | Value |
|--------|-------|
| Total Poses | 2804 |
| Keyframes | 302 |
| Map Points | ~487 (initial) |
| VIO Init | VIBA 1 + VIBA 2 ✓ |
| Tracking Failures | 0 |

---

## 🔜 Next Steps

1. **Timing Instrumentation:** Add per-frame latency measurement
2. **Accuracy Evaluation:** Compare trajectory against ground truth using EVO
3. **Harder Sequences:** Test on V1_02_medium, V1_03_difficult
4. **Optimization:** Profile and optimize for XR latency requirements (<11ms)

---
## 📚 References

- [ORB-SLAM3 Paper](https://arxiv.org/abs/2007.11898)
- [EuRoC Dataset](https://projects.asl.ethz.ch/datasets/doku.php?id=kmavvisualinertialdatasets)
- [Original ORB-SLAM3 Repo](https://github.com/UZ-SLAMLab/ORB_SLAM3)

---

## 📝 Updates - November 26, 2025

### Updated Working Command (Simplified Library Path)

The following command works correctly with simplified library paths:

```bash
cd /home/raj/Documents/Projects/xr_slam_sprint/ORB_SLAM3 && \
export LD_LIBRARY_PATH=/home/raj/Documents/Projects/system_software/pangolin/install/lib:/home/raj/Documents/Projects/system_software/lib:/home/raj/Documents/Projects/system_software/lib/opencv4/3rdparty:$LD_LIBRARY_PATH && \
./Examples/Monocular-Inertial/mono_inertial_euroc \
  ./Vocabulary/ORBvoc.txt \
  ./Examples/Monocular-Inertial/EuRoC.yaml \
  /home/raj/Documents/Projects/datasets/EuRoC/V101 \
  ./Examples/Monocular-Inertial/EuRoC_TimeStamps/V101.txt \
  dataset-V101_mono_inertial
```

**Key differences from original command:**
- Simplified `LD_LIBRARY_PATH` to point directly to install locations
- No need for `DISPLAY` variable (uses default display)
- Appends to existing `$LD_LIBRARY_PATH` rather than replacing it

---

## 📝 Updates - November 27, 2025

### Build Instructions with Tracy Profiling

After adding comprehensive Tracy profiling zones (87+ zones including RAJ_ optimization zones), the build requires explicit Eigen and OpenCV paths:

```bash
cd /home/raj/Documents/Projects/xr_slam_sprint/ORB_SLAM3/build
cmake .. \
  -DEIGEN3_INCLUDE_DIR=/home/raj/Documents/Projects/system_software/eigen/install/include/eigen3 \
  -DG2O_EIGEN3_INCLUDE=/home/raj/Documents/Projects/system_software/eigen/install/include/eigen3 \
  -DOpenCV_DIR=/home/raj/Documents/Projects/system_software/opencv/build
make -j4
```

**Why this is needed:**
- The `g2o` third-party library needs explicit `G2O_EIGEN3_INCLUDE` path
- After clean build, CMake cache doesn't retain previous Eigen configuration
- Build warnings (Eigen deprecation, unused variables) are harmless

### Tracy Profiling Zones Added

The codebase now includes comprehensive profiling for performance analysis:

**General SLAM Pipeline (87+ zones):**
- `Tracking.cc`: Track loop, IMU operations, initialization, tracking modes
- `LocalMapping.cc`: ProcessNewKeyFrame with detailed zones
- `LoopClosing.cc`: Loop detection, merging, correction
- `Optimizer.cc`: Tight zones around optimize() calls
- `Frame.cc`, `KeyFrame.cc`, `MapPoint.cc`: Core data structure operations
- `ORBextractor.cc`, `ORBmatcher.cc`: Feature extraction and matching

**SearchLocalPoints Optimization Zones (RAJ_ prefix):**
- `RAJ_MarkMatchedPoints` - Phase 1: Mark already-matched points
- `RAJ_ProjectLocalMapPoints` - Phase 2: Project ~3000 local map points (bottleneck)
- `RAJ_ComputeSearchRadius` - Adaptive search radius calculation
- `RAJ_FeatureMatching` - Descriptor matching phase
- `RAJ_isInFrustum` - Per-point visibility test (called ~3000x per frame)
- `RAJ_GetFeaturesInArea` - Spatial grid feature lookup
- `RAJ_DescriptorMatching` - Hamming distance computation loop

**Tracy Performance Plots:**
- `RAJ_VisibleMapPoints` - Points passing frustum test
- `RAJ_SearchRadius` - Search radius per frame
- `RAJ_MatchesFound` - Final matches in SearchLocalPoints
- `RAJ_CandidateMapPoints` - Candidates in SearchByProjection
- `RAJ_DescriptorComparisons` - Descriptor comparisons made
- `RAJ_FinalMatches` - Matches found by SearchByProjection

### Running with Visualization

Same command as before works with all profiling zones:

```bash
cd /home/raj/Documents/Projects/xr_slam_sprint/ORB_SLAM3 && \
export LD_LIBRARY_PATH=/home/raj/Documents/Projects/system_software/pangolin/install/lib:/home/raj/Documents/Projects/system_software/lib:/home/raj/Documents/Projects/system_software/lib/opencv4/3rdparty:$LD_LIBRARY_PATH && \
./Examples/Monocular-Inertial/mono_inertial_euroc \
  ./Vocabulary/ORBvoc.txt \
  ./Examples/Monocular-Inertial/EuRoC.yaml \
  /home/raj/Documents/Projects/datasets/EuRoC/V101 \
  ./Examples/Monocular-Inertial/EuRoC_TimeStamps/V101.txt \
  dataset-V101_mono_inertial
```

**Expected behavior:**
- Pangolin visualization windows appear (3D map view + current frame)
- Tracy profiling zones active (~0.02% overhead when profiler not connected)
- Map initializes with ~339-487 points
- Processes full V101 sequence (2804 poses, ~302 KFs)

---

## 📚 References

- [ORB-SLAM3 Paper](https://arxiv.org/abs/2007.11898)
- [EuRoC Dataset](https://projects.asl.ethz.ch/datasets/doku.php?id=kmavvisualinertialdatasets)
- [Original ORB-SLAM3 Repo](https://github.com/UZ-SLAMLab/ORB_SLAM3)
- [Tracy Profiler](https://github.com/wolfpld/tracy)
