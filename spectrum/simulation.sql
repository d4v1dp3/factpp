WITH Table0 AS
(
   SELECT
      Weight, Energy, LogEnergyEst,

      -- Convert variable to bin index
      INTERVAL(Theta, %107:theta)  AS `.theta`,
      INTERVAL(LogEnergyEst, %108:sparse)  AS `.sparse_est`,
      INTERVAL(LogEnergyEst, %109:dense)  AS `.dense_est`,
      INTERVAL(LOG10(Energy), %108:sparse)  AS `.sparse_sim`,
      INTERVAL(LOG10(Energy), %109:dense)  AS `.dense_sim`,
      INTERVAL(Impact/100, %110:impact)  AS `.impact`,

      (%111:spectrum)/POW(Energy, SpectralIndex) AS SpectralWeight,  -- FIXME: Is this correct for files with different Slopes?
      LogEnergyEst - log10(Energy) AS Residual
   FROM
      Excess
-- Instead of using %%0:columns, we could join back with the data we need
--   INNER JOIN
--      factmc.EventsMC USING(FileId, EvtNumber, CorsikaNumReuse)
--   INNER JOIN
--      factmc.RunInfoMC USING(FIleId)
)
SELECT
   `.theta`,
   `.sparse_est`,
   `.sparse_sim`,
   `.dense_est`,
   `.dense_sim`,
   `.impact`,

   -- Without any weight applied
   COUNT(IF(Weight>0, 1, NULL))  AS  SignalN,
   COUNT(IF(Weight<0, 1, NULL))  AS  BackgroundN,

   -- Without ZdWeight applied
/*
   SUM(  IF(Weight>0,     SpectralWeight,    0))  AS  Signal,
   SUM(  IF(Weight<0,     SpectralWeight,    0))  AS  Background,
   SUM(  IF(Weight>0, POW(SpectralWeight,2), 0))  AS  Signal2,
   SUM(  IF(Weight<0, POW(SpectralWeight,2), 0))  AS  Background2,
*/

   -- Binning in estimated energy: Signal, Background
   SUM(  IF(Weight>0,        ZdWeight*SpectralWeight,    0))  AS  SignalW,
   SUM(  IF(Weight<0,        ZdWeight*SpectralWeight,    0))  AS  BackgroundW,
   SUM(  IF(Weight>0, POW(ErrZdWeight*SpectralWeight,2), 0))  AS  SignalW2,
   SUM(  IF(Weight<0, POW(ErrZdWeight*SpectralWeight,2), 0))  AS  BackgroundW2,

   -- Energy Estimation: Bias=ResidualW/SignalW; Resolution=sigma(Bias)
   SUM(IF(Weight>0,     Residual*   ZdWeight*SpectralWeight, 0))  AS  ResidualW,
   SUM(IF(Weight>0, POW(Residual,2)*ZdWeight*SpectralWeight, 0))  AS  ResidualW2,

   -- Average Energy: SumEnergyX/SignalW
   SUM(IF(Weight>0, Energy               *ZdWeight*SpectralWeight, 0))  AS  SumEnergySimW,
   SUM(IF(Weight>0, POW(10, LogEnergyEst)*ZdWeight*SpectralWeight, 0))  AS  SumEnergyEstW

FROM
   Table0
INNER JOIN
   ThetaDist USING(`.theta`)
GROUP BY
   `.theta`, `.sparse_est`, `.sparse_sim`, `.dense_est`, `.dense_sim`, `.impact`
