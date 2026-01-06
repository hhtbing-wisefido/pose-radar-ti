# Microsoft PowerPoint - AWRL6844_SBRCPD_pointCloud_TuningGuide

> **源文件名**: AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide.pdf
> **源文件绝对路径**: `C:\ti\radar_toolbox_3_30_00_06\source\ti\examples\Automotive_InCabin_Security_and_Safety\AWRL6844_InCabin_Demos\docs\AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide.pdf`
> **作者**: Yang, Zigang
> **PDF标题**: Microsoft PowerPoint - AWRL6844_SBRCPD_pointCloud_TuningGuide
> **页数**: 11
> **文件大小**: 1196.1 KB
> **转换时间**: 2025-12-25 14:11:05
> **提取图片**: 16 张

---

## AWRL6844 Real-Time Demo:

## Low Power Radar - Body and Chassis

TI Information – Selective Disclosure

![图片 1-1 (1920x540)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p1_img1.jpeg)

![图片 1-2 (1920x540)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p1_img2.jpeg)

![图片 1-4 (1017x465)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p1_img4.jpeg)

<!-- 第 1 页结束 -->

## Step 1: Phase Calibration

### • Applying phase calibration is crucial to improve system performance.

– Users can find the instruction at the SDK L users guide at:

https://dev.ti.com/tirex/explore/content/MMWAVE_L_SDK$_{06}$$_{00}$$_{05}$$_{01}$/docs/api_guide_xwrL684x/MMWAVE_DE
MO.html#Rx_Gain_Measurement_Compensation

TI Information – Selective Disclosure

![图片 2-2 (1539x626)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p2_img2.jpeg)

<!-- 第 2 页结束 -->

## Step 2: Carefully Choose the Mounting Position and Angle

### Performance can be sensitive to mounting:  Observe better performance when the mounting is

### • To support different mounting position and angle, "sensorPosition" CLI command is used

– Indicates the mounting offset in (x, y, z) and mounting rotation angle in x-y plane and x-z plane

CLI command
Parameters (in command order)

sensorPosition

xOffset
offset in x direction, in meter

azimTilt
Counter-Clockwise rotation angle in x-y plane, in degree

zOffset
offset in z direction, in meter

yOffset
offset in y direction, in meter

elevTilt
Counter-Clockwise rotation angle in x-z plane, in degree

TI Information – Selective Disclosure

![图片 3-2 (1037x625)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p3_img2.jpeg)

<!-- 第 3 页结束 -->

## Step 2-1: Program sensorPosition CLI to Match the Mounting

### Front Mount

### Middle Overhead Mounting

(x = 0, y = 0.4, z = 0.8m)
and facing forward
without any rotation

(x = 0, y = 1.2m, z = 1.1m)
rotated down 90 degrees
to face the floor

### Mounting

### Front Console Mount

### Front Console Mount with offset

(x = 0, y = 0.7m, z = 1.08m)
rotated down 60 degrees
from the forward position

(x = -0.1, y = 0.7m, z = 1.08m)
rotated down 60 degrees
from the forward position

TI Information – Selective Disclosure

![图片 4-2 (249x264)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p4_img2.jpeg)

![图片 4-3 (280x264)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p4_img3.jpeg)

![图片 4-4 (250x264)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p4_img4.jpeg)

![图片 4-5 (249x263)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p4_img5.jpeg)

<!-- 第 4 页结束 -->

## Step 2-2: Understand Antenna Performance

### We observed an asymmetric antenna performance on the AWRL6844EVM

### roughly based on the regular front mounting.

### in seat 3.

### Seat 3

### Driver

TI Information – Selective Disclosure

![图片 5-2 (931x641)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p5_img2.jpeg)

![图片 5-3 (359x371)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p5_img3.jpeg)

<!-- 第 5 页结束 -->

## Step 2-3: Consider Rotating EVM to Reduce Antenna

### We have observed slightly better performance in the CPD and SBR demo when applying an

### alternate antenna mounting position shown below. We have been using this alternate

Regular mounting
sensorPosition 0 0.7 1.05 0 -60

Rotate EVM 180 degrees,
sensorPosition 0 0.7 1.05 180 -120

TI Information – Selective Disclosure

![图片 6-2 (395x408)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p6_img2.jpeg)

![图片 6-3 (513x583)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p6_img3.jpeg)

<!-- 第 6 页结束 -->

## Step 3: Adapt to a New Testing Car

TI Information – Selective Disclosure

![图片 7-2 (1647x886)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p7_img2.jpeg)

![图片 7-3 (1238x571)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p7_img3.jpeg)

<!-- 第 7 页结束 -->

## Step 3-1: Fine adjustment to the zone definition

### •

### Rotate the point cloud in the

TI Information – Selective Disclosure

![图片 8-2 (1235x649)](AWRL6844_CPD_&_SBR_Point_Cloud_Tuning_Guide_images/AWRL6844_CPD_%26_SBR_Point_Cloud_Tuning_Guide_p8_img2.jpeg)

<!-- 第 8 页结束 -->

## Step 4: Tune the point cloud through CFAR parameter

### Starting parameter for SBR:

dynamicRACfarCfg
5 15 1 1 8 8 4 6 4 1 8.00 6.00 0.50 1 15


If CFAR threshold K0 (on range dimention) is set too high, little/no point cloud can be obtained
with the weak target (such as baby rear-facing scene or baby in footwell);  Reduce this value to
get more point clouds detected especially for the weak target

If CFAR threshold K0 (on range dimention) is set too low, an excessive point clouds can be
observed on non-occupied seats (due to multipath or noise)

CFAR Threshold K1 (on angle dimension): not very sensitive to weak or strong target detection.

Average window: Prefer to set to 8 to have fewer false detections due to the noise

Guard interval: Setting it longer can be helpful to detect a group of continuous peaks (an example
here: the green bin can’t be detected if the guard interval is less than 4).

Sidelobe threshold: multiple angle peaks in the same range bin, a relative threshold for detecting
the lower peak. (As an example, the green angle bin will not be detected if its relative energy level
to the blue angle bin is lower than the sidelobe threshold) This parameter can affect multi-target
detection in the same range bin.

rangeRefIdx to apply Dynamic threshold: the range threshold will be dynamically increased for the
range bins between (1, rangeRefIdx)

TI Information – Selective Disclosure

<!-- 第 9 页结束 -->

## Step 4-1: Tune the setting in dynamicRACfarCfg

CFAR parameters
Description
Default setting
SBR
CPD

leftSkipSizeRange
range bin start index (first range bin processed by CFAR)
5
Default
Default

rightSkipSizeRange
range bin end skip (ending range bins not processed by CFAR)
15
Default
10

leftSkipSizeAngle
angle start index (first angle bin processed by CFAR)
1
Default
Default

rightSkipSizeAngle
angle end skip (ending angle bins not processed by CFAR)
1
Default
Default

searchWinSizeRange
search window size - “far” range
8
Default
Default

searchWinSizeAngle
search window size - angle
8
Default
Default

searchWinSizeNear
search window size - “near” range
=searchWinSizeRange
Set to 4
Default

guardSizeRange
guard window size - “far” range
6
Default
6 ~ 10

guardSizeAngle
guard window size - angle
4
Default
Default

guardSizeNear
guard window size - “near” range
= guardSizeRange
Set to 1
Default

threRange
“K0” cross range detection threshold applied to first search pass
8.0
Default
7.5 or 7

threAngle
“K0” cross angle detection threshold used in the 2nd search pass
6.0
Default
6.0 or 5.5

threSidelobe
“sidelobe” threshold used in the second search pass
0.5
Default
Default

enSecondPass
second search pass enable flag
1
Default
Default

rangeRefIndex
The CFAR threshold “K0” will be adjusted for shorter range bins
based on K0*(refRangeBinIdx/rangeBinIdx)$^{2}$.

15
Default
1: disable
dynamic CFAR

TI Information – Selective Disclosure

<!-- 第 10 页结束 -->

## Step 4-2: Tune the setting in dynamic2DAngleCfg

CFAR parameters
Description
Default
setting

### dynamic2DAngleCfg 5 1 1 1.00 10.00 2

SBR
CPD

zoominFactor
zoom-in Factor for finer angle estimation: 5 is recommended
5
Default
Default

zoominNumOfNeighbors
Number of coarse neighboring angle bins of zoom-in. Only 1 is
supported

1
Default
Default

peakExpSamples
Number of samples on each side to expand the peak of the
zoomed-in azimuth-elevation heatmap, 1 is recommended.

peakExpRelThr
The relative threshold in the linear scale to include the peak's
neighbor as a detection. This parameter is calculated based on
the sharpness of the peak.

1
Default
Default

1. 0
2. Default
3. Default

peakExpSNRThr
Linear SNR threshold for the peak to enable the peak
expansion in the zoomed-in heatmap.

10.0
Default
Set to 7.0 or above
to allow more peak
expansion

localMaxCheckFlag
0: No local maximum check. 1: If the coarse peak is not a local
maximum in the elevation domain, exclude it from the
detection. 2: If the coarse peak is not a local maximum in both
elevation and azimuth domains, exclude it from the detection.

2
Default
Default

TI Information – Selective Disclosure

<!-- 第 11 页结束 -->
