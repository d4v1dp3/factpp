WITH Theta AS -- Get total Observation time
(
   SELECT SUM(OnTime) AS ObsTime FROM ThetaDist
),
Area AS -- Get total simulated area
(
   SELECT POW(MinImpactHi,2)*PI() AS Area FROM MonteCarloArea
),
Binning AS -- Create binning and calculate weights per bin
(
   SELECT
      *,
      (%103:binwidth)  AS  Width,   -- Bin Windth in TeV or just 1
      Area*ObsTime  AS  AreaTime    -- Total scale dA*dT
   FROM
      %101:binning
   CROSS JOIN
      Theta, Area
),
Simulated AS -- Summarize simulated (corsika) data in bins
(
   SELECT
      %102:bin,
      SUM(CountN) AS SimCountN,
      SUM(SumW)   AS SimSumW,
      SUM(SumW2)  AS SimSumW2
   FROM
      WeightedOriginalMC
   GROUP BY
      %102:bin
),
Triggered AS -- Summarize triggered (ceres) data in bins
(
   SELECT
      %102:bin,
      SUM(CountN) AS TrigCountN,
      SUM(SumW)   AS TrigSumW,
      SUM(SumW2)  AS TrigSumW2
   FROM
      WeightedEventsMC
   GROUP BY
      %102:bin
),
Analyzed AS -- Summarize data after cuts (analyzed) in bins
(
   SELECT
      %102:bin,

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
      %102:bin
),
CombinedData AS -- Combine all Data together
(
   SELECT
      *,
      TrigCountN/SimCountN     AS  TriggerEfficiencyN,
      TrigSumW/SimSumW         AS  TriggerEfficiencyW,

      SignalW  - BackgroundW   AS  ExcessW,
      SignalW2 + BackgroundW2  AS  ExcessW2
   FROM
      Simulated
   INNER JOIN
      Triggered USING (%102:bin)
   INNER JOIN
      Analyzed USING (%102:bin)
   INNER JOIN
      Binning ON Simulated.%102:bin=bin
),
Table0 AS   -- Derive valued
(
   SELECT   -- Everything scaled is "Flux", Everything unscaled is without "Flux", Corsika Data is Sim*, Triggered Data is Trig* and data after cuts is without prefix
      *,

      SQRT(SimCountN)                        AS  ErrSimCountN,
      SQRT(TrigCountN)                       AS  ErrTrigCountN,
      SQRT(SignalN)                          AS  ErrSignalN,
      SQRT(BackgroundN)                      AS  ErrBackgroundN,

      SimSumW              /Width/AreaTime   AS  SimFluxW,
      TrigSumW             /Width/AreaTime   AS  TrigFluxW,
      SignalW              /Width/AreaTime   AS  SignalFluxW,
      BackgroundW          /Width/AreaTime   AS  BackgroundFluxW,
      ExcessW              /Width/AreaTime   AS  ExcessFluxW,

      SQRT(SimSumW2)       /Width/AreaTime   AS  ErrSimFluxW,
      SQRT(TrigSumW2)      /Width/AreaTime   AS  ErrTrigFluxW,
      SQRT(SignalW2)       /Width/AreaTime   AS  ErrSignalFluxW,
      SQRT(BackgroundW2)   /Width/AreaTime   AS  ErrBackgroundFluxW,
      SQRT(ExcessW2)       /Width/AreaTime   AS  ErrExcessFluxW,

      SumEnergyEstW/SignalW                  AS  AvgEnergyEstW,
      SumEnergySimW/SignalW                  AS  AvgEnergySimW,

      Area*TriggerEfficiencyN                AS  EffectiveAreaN,
      Area*TriggerEfficiencyW                AS  EffectiveAreaW,

      SignalN/TrigCountN                     AS  CutEfficiencyN,
      IF(ExcessW<0, 0, ExcessW/TrigSumW)     AS  CutEfficiencyW,

      TriggerEfficiencyN * SQRT(1/TrigCountN + 1/SimCountN)                          AS  ErrTriggerEfficiencyN,
      TriggerEfficiencyW * SQRT(TrigSumW2/POW(TrigSumW,2) + SimSumW2/POW(SimSumW,2)) AS  ErrTriggerEfficiencyW,

      IF(SignalW>0, ResidualW/SignalW,                                                             NULL)  AS  BiasW,
      IF(SignalW>0, ResidualW/SignalW*SQRT(ResidualW2/POW(ResidualW,2) + SignalW2/POW(SignalW,2)), NULL)  AS  ErrBiasW,
      IF(SignalW>0, SQRT(ResidualW2/SignalW - POW(ResidualW/SignalW, 2)),                          NULL)  AS  ResolutionW,

      (SUM(SignalW)       OVER Integral)                     AS  IntegralSignalW,
      (SUM(SignalW)       OVER Integral) / AreaTime          AS  IntegralSignalFluxW,
      (SUM(BackgroundW)   OVER Integral) / AreaTime          AS  IntegralBackgroundFluxW,
      (SUM(SignalW2)      OVER Integral) / POW(AreaTime, 2)  AS  IntegralSignalFluxW2,
      (SUM(BackgroundW2)  OVER Integral) / POW(AreaTime, 2)  AS  IntegralBackgroundFluxW2,

      (SUM(SimSumW)       OVER Integral) / AreaTime          AS  IntegralSimFluxW,
      (SUM(SimSumW2)      OVER Integral) / POW(AreaTime, 2)  AS  IntegralSimFluxW2,

      (SUM(SumEnergySimW) OVER Integral)                     AS  IntegralEnergySimW,
      (SUM(SumEnergyEstW) OVER Integral)                     AS  IntegralEnergyEstW
   FROM
      CombinedData
   WINDOW
      Integral AS (ORDER BY %102:bin DESC)
)
SELECT -- Return derived values and result
   *,

   Area*ErrTriggerEfficiencyN  AS  ErrEffectiveAreaN,
   Area*ErrTriggerEfficiencyW  AS  ErrEffectiveAreaW,

   CutEfficiencyN*SQRT(1/SignalN + 1/TrigCountN)                             AS  ErrCutEfficiencyN,
   CutEfficiencyW*SQRT(ExcessW2/POW(ExcessW,2) + TrigSumW2/POW(TrigSumW,2))  AS  ErrCutEfficiencyW,

   IntegralSignalFluxW - IntegralBackgroundFluxW        AS  IntegralExcessFluxW,
   SQRT(IntegralSignalFluxW2+IntegralBackgroundFluxW2)  AS  ErrIntegralExcessFluxW,
   SQRT(IntegralSignalFluxW2)                           AS  ErrIntegralSignalFluxW,
   SQRT(IntegralBackgroundFluxW2)                       AS  ErrIntegralBackgroundFluxW,
   SQRT(IntegralSimFluxW2)                              AS  ErrIntegralSimFluxW,

   IntegralEnergyEstW/IntegralSignalW                   AS  AvgIntegralEnergyEstW,
   IntegralEnergySimW/IntegralSignalW                   AS  AvgIntegralEnergySimW

FROM
   Table0
