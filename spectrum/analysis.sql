/* ************************************************************************

   This is the analysis query. It returns two columns for all
   signal/background events.

     - `Weight` (Positive for signal (typ. 1),
                 negative for background (typ. -0.2))
     - `LogEnergyEst` logarithm of estimated energy in GeV

   In additon, all columns provided by the 100-clause must be
   returned. (Note that you must not add a comma behind it)

   100| %100:files:: table containing the `FileId`s to analyze.
   101| %101:runinfo:: table with the run info data
   102| %102:events:: table with the image parameters
   103| %103:positions:: table with the source positions in the camera
   104| %105:zenith:: zenith angle in degrees
   105| %104:columns
   106| %105:estimator:: estimator for log10 energy

   WARNING:
     Right now, we correlate the mean zenith angle of the data
     file with the particle direction in the simulation!

*************************************************************************** */

WITH Table0 AS
(
   SELECT
      %105:columns  -- this could be removed if we can join events via the same columns (without CorsikaNumResuse)
      %104:zenith AS Theta,
      Weight,
      Size,
      NumUsedPixels,
      NumIslands,
      Leakage1,
      MeanX,
      MeanY,
      CosDelta,
      SinDelta,
      M3Long,
      SlopeLong,
      Width/Length      AS WdivL,
      PI()*Width*Length AS Area,
      cosa*X - sina*Y   AS PX,
      cosa*Y + sina*X   AS PY
   FROM
      %100:files
   LEFT JOIN
      %101:runinfo USING (FileId)
   LEFT JOIN 
      %102:events USING (FileId)  -- This could be replaced by a user uploaded temporary table
   LEFT JOIN 
      %103:positions USING (FileId, EvtNumber)
   CROSS JOIN 
      Wobble
   WHERE
      NumUsedPixels>5.5
   AND
      NumIslands<3.5
   AND
      Leakage1<0.1
),

Table1 AS
(
   SELECT
      %105:columns
      Theta, Weight,
      Size, CosDelta, SinDelta, M3Long, SlopeLong, Leakage1, WdivL,
      MeanX - PX/1.02e0 AS DX,
      MeanY - PY/1.02e0 AS DY
   FROM
      Table0
   WHERE
      Area < LOG10(Size)*898e0 - 1535e0
),

Table2 AS
(
   SELECT
      %105:columns
      Theta, Weight,
      Size, CosDelta, SinDelta, DX, DY, M3Long, SlopeLong, Leakage1, WdivL,
      SQRT(DX*DX + DY*DY) AS Norm
   FROM
      Table1
),

Table3 AS
(
   SELECT
      %105:columns
      Theta, Weight,
      Size, M3Long, SlopeLong, Leakage1, WdivL, Norm,
      LEAST(GREATEST((CosDelta*DY - SinDelta*DX)/Norm, -1), 1) AS LX,
      SIGN(CosDelta*DX + SinDelta*DY) AS Sign
   FROM
      Table2
),

Table5 AS
(
   SELECT
      %105:columns
      Theta, Weight,
      Size, Leakage1, WdivL, LX,
      Norm          *0.0117193246260285378e0 AS Dist,
      M3Long   *Sign*0.0117193246260285378e0 AS M3L,
      SlopeLong*Sign/0.0117193246260285378e0 AS Slope
   FROM
      Table3
),

Table6 AS
(
   SELECT
      %105:columns
      Theta, Weight,
      Size, WdivL, Dist, LX, M3L, Slope,
      1.39252e0 + 0.154247e0*Slope + 1.67972e0*(1-1/(1+4.86232e0*Leakage1)) AS Xi
   FROM
      Table5
),

Table7 AS
(
   SELECT
      %105:columns
      Theta, Weight,
      Size, Dist, LX,
      IF (M3L<-0.07 OR (Dist-0.5e0)*7.2e0-Slope<0, -Xi, Xi) * (1-WdivL) AS Disp
   FROM
      Table6
)

SELECT
   %105:columns
   Theta, Weight,
   (Disp*Disp + Dist*Dist - 2*Disp*Dist*SQRT(1-LX*LX)) AS ThetaSq,
   %106:estimator AS LogEnergyEst
FROM
   Table7
HAVING
   ThetaSq<0.024
