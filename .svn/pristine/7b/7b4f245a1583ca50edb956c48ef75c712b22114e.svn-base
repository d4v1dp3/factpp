WITH Theta AS  -- Get Total Observation Time
(
   SELECT SUM(OnTime) AS ObsTime FROM ThetaDist
),
Area AS -- Get total simulated area
(
   SELECT POW(MinImpactHi,2)*PI() AS Area FROM MonteCarloArea
),
Binning AS -- Get binning and calculate weights for each bin
(
   SELECT
      *,
      (POW(10,hi)-POW(10,lo))/1000  AS  Width,    -- Bin Windth in TeV
      Area*ObsTime                  AS  AreaTime  -- Total scale dA*dT
   FROM
      BinningEnergy_%100:binning
   CROSS JOIN
      Theta, Area
),
Analyzed AS -- Summarizy data after cuts (analyzed) in bins
(
   SELECT
      `.%100:binning:_est`,

      SUM(SignalN)          AS  SignalN,
      SUM(SignalW)          AS  SignalW,
      SUM(SignalW2)         AS  SignalW2,

      SUM(BackgroundN)/5    AS  BackgroundN,
      SUM(BackgroundW)/5    AS  BackgroundW,
      SUM(BackgroundW2)/25  AS  BackgroundW2,

      SUM(ResidualW)        AS  ResidualW,
      SUM(ResidualW2)       AS  ResidualW2,

      SUM(SumEnergySimW)    AS  SumEnergySimW,
      SUM(SumEnergyEstW)    AS  SumEnergyEstW
   FROM
      AnalysisMC
   GROUP BY
      `.%100:binning:_est`
),
Integrated AS
(
   SELECT -- Return Result
      *,

      SignalW                       /Width/AreaTime  AS  SignalFluxW,
      BackgroundW                   /Width/AreaTime  AS  BackgroundFluxW,
      (SignalN - BackgroundN)                        AS  ExcessN,
      (SignalW - BackgroundW)                        AS  ExcessW,
      (SignalW - BackgroundW)       /Width/AreaTime  AS  ExcessFluxW,

      SQRT(SignalN)                                  AS  ErrSignalN,
      SQRT(SignalW2)                                 AS  ErrSignalW,
      SQRT(SignalW2)                /Width/AreaTime  AS  ErrSignalFluxW,

      SQRT(BackgroundN)                              AS  ErrBackgroundN,
      SQRT(BackgroundW2)                             AS  ErrBackgroundW,
      SQRT(BackgroundW2)            /Width/AreaTime  AS  ErrBackgroundFluxW,

      SQRT(SignalN + BackgroundN)                    AS  ErrExcessN,
      SQRT(SignalW2 + BackgroundW2)                  AS  ErrExcessW,
      SQRT(SignalW2 + BackgroundW2) /Width/AreaTime  AS  ErrExcessFluxW,

      SumEnergyEstW/SignalW                          AS  AvgEnergyEstW,
      SumEnergySimW/SignalW                          AS  AvgEnergySimW,

      IF(SignalW>0, ResidualW/SignalW,                                                             NULL)  AS  BiasW,
      IF(SignalW>0, ResidualW/SignalW*SQRT(ResidualW2/POW(ResidualW,2) + SignalW2/POW(SignalW,2)), NULL)  AS  ErrBiasW,
      IF(SignalW>0, SQRT(ResidualW2/SignalW - POW(ResidualW/SignalW, 2)),                          NULL)  AS  ResolutionW,

      (SUM(SignalW)       OVER Integral)                     AS  IntegralSignalW,
      (SUM(SignalW)       OVER Integral) / AreaTime          AS  IntegralSignalFluxW,
      (SUM(BackgroundW)   OVER Integral) / AreaTime          AS  IntegralBackgroundFluxW,
      (SUM(SignalW2)      OVER Integral) / POW(AreaTime, 2)  AS  IntegralSignalFluxW2,
      (SUM(BackgroundW2)  OVER Integral) / POW(AreaTime, 2)  AS  IntegralBackgroundFluxW2,

      (SUM(SumEnergySimW) OVER Integral)                     AS  IntegralEnergySimW,
      (SUM(SumEnergyEstW) OVER Integral)                     AS  IntegralEnergyEstW
   FROM
      Analyzed
   INNER JOIN
      Binning ON `.%100:binning:_est`=bin
   WINDOW
      Integral AS (ORDER BY `.%100:binning:_est` DESC)
)
SELECT
   *,

   IntegralSignalFluxW-IntegralBackgroundFluxW          AS  IntegralExcessFluxW,
   SQRT(IntegralSignalFluxW2+IntegralBackgroundFluxW2)  AS  ErrIntegralExcessFluxW,
   SQRT(IntegralSignalFluxW2)                           AS  ErrIntegralSignalFluxW,
   SQRT(IntegralBackgroundFluxW2)                       AS  ErrIntegralBackgroundFluxW,

   IntegralEnergyEstW/IntegralSignalW                   AS  AvgIntegralEnergyEstW,
   IntegralEnergySimW/IntegralSignalW                   AS  AvgIntegralEnergySimW

FROM
   Integrated
