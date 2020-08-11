SELECT
   -- Convert variable to bin index
   INTERVAL(Theta, %107:theta)  AS `.theta`,
   INTERVAL(LogEnergyEst, %108:sparse)  AS  `.sparse_est`,

   -- Signal and Background counts
   COUNT(IF(Weight>0, 1, NULL))  AS  `Signal`,
   COUNT(IF(Weight<0, 1, NULL))  AS  `Background`,

   -- Average Energy: SumEnergyEst/SumW
   SUM(Weight*POW(10, LogEnergyEst))  AS  SumEnergyEst,
   SUM(Weight)                        AS  SumW
FROM
   Excess
GROUP BY
   `.theta`, `.sparse_est`
ORDER BY
   `.theta`, `.sparse_est`
