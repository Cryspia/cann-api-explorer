# Div -- simulation result

- Status: **passed**
- Build SOC: `Ascend950PR_9599`  Run SOC: `Ascend950`
- PASS marker: `DIV SIMULATION PASSED`
- Instruction count: 30901
- Execution time (ns): 6040.0

## record log (tail)
```
INFO:root:Report INFO
INFO:root:Analysis type: hotspots
INFO:root:Output folder: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/div/report/cannsim_20260611184951_sim_exe/report/results_20260612_025037938033
INFO:root:Generated Architecture Diagrams: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/div/report/cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports
INFO:root:Chrome Tracing reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/div/report/cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports
INFO:root:Critical path reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/div/report/cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports
INFO:root:Issue Queue reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/div/report/cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports
INFO:root:Scalar IPC dynamic reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/div/report/cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports
INFO:root:Vector IPC dynamic reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/div/report/cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports
INFO:root:Instruction Cache reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/div/report/cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports
INFO:root:====================
INFO:root:Functions INFO
INFO:root:Kernel name: kernel
INFO:root:Total number of instructions: 0
INFO:root:Total number of instruction executions: 30901
INFO:root:Execution time (ns): 6040.0
INFO:root:====================
[2026-06-11 18:50:37] [INFO] Executing cannprof report...
[2026-06-11 18:50:37] [INFO] Using cannprof binary: /home/spark/miniforge3/envs/cannsim/cann/cann-9.1.0-beta.1/python/site-packages/cannsim/bin/cannprof
[2026-06-11 18:50:39] [INFO] Report generation completed successfully.
+==========================================================================+
│ (Report) Cannsim auto generate report successed!                         │
+==========================================================================+
Cannsim auto generate report saved at:
 └─/home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/div/report/cannsim_20260611184951_sim_exe/report
```

## Report artifacts
```
cannsim_20260611184951_sim_exe/cannsim.log
cannsim_20260611184951_sim_exe/instr.bin
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_operator.svg
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_request.svg
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/aicore_memory_sim_view_number_of_requests.svg
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/aicore_utilization.json
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_0_critical_path_report_0.json
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_0_icache_report.html
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_0_scalar_ipc_dynamic_report.html
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_0_tracing_report_0.json
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_16_critical_path_report_0.json
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_16_icache_report.html
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_16_scalar_ipc_dynamic_report.html
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_16_tracing_report_0.json
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_17_critical_path_report_0.json
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_17_icache_report.html
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_17_scalar_ipc_dynamic_report.html
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_17_tracing_report_0.json
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_1_critical_path_report_0.json
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_1_icache_report.html
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_1_scalar_ipc_dynamic_report.html
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/core_1_tracing_report_0.json
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/kernel_0_reports/instruction_duration_distribution.html
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/perf_log.dump
cannsim_20260611184951_sim_exe/report/results_20260612_025037938033/runner.py.log
cannsim_20260611184951_sim_exe/route_table.txt
cannsim_20260611184951_sim_exe/rtb_debug.txt
cannsim_20260611184951_sim_exe/.soc-version
record.log
```
