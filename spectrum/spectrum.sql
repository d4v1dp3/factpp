WITH BinnedData AS -- Bin data and create sums per bin
(
   SELECT
      %101:bin,

      SUM(`Signal`)        AS  `Signal`,
      SUM(`Background`)/5  AS  `Background`,
      SUM(SumEnergyEst)    AS  SumEnergyEst,
      SUM(SumW)            AS  SumW
   FROM
      AnalysisData
   GROUP BY
      %101:bin
),
Data AS -- Integral is a placeholder for the bin and all following bins
(
   SELECT
      *,
      SUM(`Signal`)     OVER Integral  AS  SignalI,
      SUM(`Background`) OVER Integral  AS  BackgroundI,
      SUM(SumEnergyEst) OVER Integral  AS  EnergyEstI,
      SUM(SumW)         OVER Integral  AS  SumWI
   FROM
      BinnedData
   WINDOW
      Integral AS (ORDER BY %101:bin DESC)
),
CombinedData AS -- Joind data together and for conveninece (easier reading of the query) rename the columns
(
   SELECT
      %102:id.%101:bin,
      %102:id.lo,
      %102:id.hi,
      (%102:id.lo+%102:id.hi)/2  AS  center,
      %102:id.Width,
      %102:id.AreaTime,

      `Signal`,
      `Background`,

      SignalI,
      BackgroundI,

      `Signal` - `Background`         AS  Excess,
      SQRT(`Signal`)                  AS  ErrSignal,
      SQRT(`Background`/5)            AS  ErrBackground,
      ExcErr(`Signal`, `Background`)  AS  ErrExcess,
      LiMa(  `Signal`, `Background`)  AS  Significance,

      SignalI - BackgroundI           AS  ExcessI,
      SQRT(SignalI)                   AS  ErrSignalI,
      SQRT(BackgroundI/5)             AS  ErrBackgroundI,
      ExcErr(SignalI, BackgroundI)    AS  ErrExcessI,
      LiMa(  SignalI, BackgroundI)    AS  SignificanceI,

      SumEnergyEst/SumW               AS  AvgEnergyEst,
      EnergyEstI/SumWI                AS  AvgEnergyEstI,

      %102:id.ExcessFluxW                 AS  SimExcess,
      %102:id.ErrExcessFluxW              AS  ErrSimExcess,
      %102:id.IntegralExcessFluxW         AS  SimExcessI,
      %102:id.ErrIntegralExcessFluxW      AS  ErrSimExcessI,

      -- For flux-vs-theta: Correction for already applied ZdWeights
      Sim.SimFluxW/%103:weight  AS  SimFluxW,
      Sim.ErrSimFluxW/%104:errweight  AS  ErrSimFluxW,
      Sim.IntegralSimFluxW/%103:weight  AS  SimFluxI,
      Sim.ErrIntegralSimFluxW/%104:errweight  AS  ErrSimFluxI
   FROM
      Data
   INNER JOIN
      %105:join1
   INNER JOIN
      %106:join2
),
Flux AS
(
   SELECT -- Calculate Flux and Relative Errors
      *,

      -- Differential Spectrum

      SimExcess/SimFluxW  AS  Efficiency,

      Excess/SimExcess/Width/AreaTime  AS  ExcessRatio,
         SQRT(
             + POW(ErrExcess    / Excess,    2)
             + POW(ErrSimExcess / SimExcess, 2)
         )  AS  RelErrExcessRatio,


      Excess/SimExcess*SimFluxW/Width/AreaTime  AS  Flux,
         SQRT(
             + POW(ErrExcess    / Excess,    2)
             + POW(ErrSimExcess / SimExcess, 2)
             + POW(ErrSimFluxW  / SimFluxW,  2)
         )  AS  RelErrFlux,

      -- Integral Spectrum

      SimExcessI/SimFluxI  AS  EfficiencyI,

      ExcessI/SimExcessI/AreaTime  AS  ExcessRatioI,
         SQRT(
             + POW(ErrExcessI    / ExcessI,    2)
             + POW(ErrSimExcessI / SimExcessI, 2)
         )  AS  RelErrExcessRatioI,


      ExcessI/SimExcessI*SimFluxI/AreaTime  AS  FluxI,
         SQRT(
             + POW(ErrExcessI    / ExcessI,    2)
             + POW(ErrSimExcessI / SimExcessI, 2)
             + POW(ErrSimFluxI   / SimFluxI,   2)
         )  AS  RelErrFluxI

   FROM
      CombinedData
),
Errors AS
(
   SELECT -- Calculate Inverse of Relative Errors (Sigma) and Absolute Errors
      *,

      IF(RelErrExcessRatio =0, NULL, 1/RelErrExcessRatio )  AS  SigmaExcessRatio,
      IF(RelErrExcessRatioI=0, NULL, 1/RelErrExcessRatioI)  AS  SigmaExcessRatioI,
      IF(RelErrFlux        =0, NULL, 1/RelErrFlux        )  AS  SigmaFlux,
      IF(RelErrFluxI       =0, NULL, 1/RelErrFluxI       )  AS  SigmaFluxI,

      IF(Excess =0, ErrExcess /SimExcess   /AreaTime/Width, ExcessRatio *RelErrExcessRatio )  AS  ErrExcessRatio,
      IF(ExcessI=0, ErrExcessI/SimExcessI  /AreaTime,       ExcessRatioI*RelErrExcessRatioI)  AS  ErrExcessRatioI,
      IF(Excess =0, ErrExcess /Efficiency  /AreaTime/Width, ABS(Flux)   *RelErrFlux        )  AS  ErrFlux,
      IF(ExcessI=0, ErrExcessI/EfficiencyI /AreaTime,       ABS(FluxI)  *RelErrFluxI       )  AS  ErrFluxI
   FROM
      Flux
)
SELECT
   *,

   -- Integrate Differential Spectrum

   SUM(Flux*Width) OVER Integral AS IntegratedFlux,
   SQRT(SUM(POW(ErrFlux*Width,2)) OVER Integral) AS ErrIntegratedFlux

FROM
   Errors
WINDOW
   Integral AS (ORDER BY %101:bin DESC)
ORDER BY
   %101:bin ASC
