# Select -- simulation result

- Status: **passed**
- Host arch: `aarch64`
- Build SOC: `Ascend950PR_9599`  Run SOC: `Ascend950`
- PASS marker: `SELECT SIMULATION PASSED`
- Instruction count: 39450
- Execution time (ns): 6865.45

## record log (tail)
```
INFO:root:Report INFO
INFO:root:Analysis type: hotspots
INFO:root:Output folder: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/select/report/cannsim_20260614201022_sim_exe/report/results_20260615_041114057628
INFO:root:Generated Architecture Diagrams: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/select/report/cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports
INFO:root:Chrome Tracing reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/select/report/cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports
INFO:root:Critical path reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/select/report/cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports
INFO:root:Issue Queue reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/select/report/cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports
INFO:root:Scalar IPC dynamic reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/select/report/cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports
INFO:root:Vector IPC dynamic reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/select/report/cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports
INFO:root:Instruction Cache reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/select/report/cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports
INFO:root:====================
INFO:root:Functions INFO
INFO:root:Kernel name: kernel
INFO:root:Total number of instructions: 0
INFO:root:Total number of instruction executions: 39450
INFO:root:Execution time (ns): 6865.45
INFO:root:====================
[2026-06-14 20:11:13] [INFO] Executing cannprof report...
[2026-06-14 20:11:13] [INFO] Using cannprof binary: /home/spark/miniforge3/envs/cannsim/cann/cann-9.1.0-beta.1/python/site-packages/cannsim/bin/cannprof
[2026-06-14 20:11:16] [INFO] Report generation completed successfully.
+==========================================================================+
│ (Report) Cannsim auto generate report successed!                         │
+==========================================================================+
Cannsim auto generate report saved at:
 └─/home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/select/report/cannsim_20260614201022_sim_exe/report
```

## Report artifacts
```
cannsim_20260614201022_sim_exe/cannsim.log
cannsim_20260614201022_sim_exe/instr.bin
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_operator.svg
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_request.svg
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/aicore_memory_sim_view_number_of_requests.svg
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/aicore_utilization.json
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_0_critical_path_report_0.json
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_0_icache_report.html
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_0_scalar_ipc_dynamic_report.html
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_0_tracing_report_0.json
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_16_critical_path_report_0.json
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_16_icache_report.html
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_16_scalar_ipc_dynamic_report.html
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_16_tracing_report_0.json
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_17_critical_path_report_0.json
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_17_icache_report.html
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_17_scalar_ipc_dynamic_report.html
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_17_tracing_report_0.json
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_1_critical_path_report_0.json
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_1_icache_report.html
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_1_scalar_ipc_dynamic_report.html
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/core_1_tracing_report_0.json
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/kernel_0_reports/instruction_duration_distribution.html
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/perf_log.dump
cannsim_20260614201022_sim_exe/report/results_20260615_041114057628/runner.py.log
cannsim_20260614201022_sim_exe/route_table.txt
cannsim_20260614201022_sim_exe/rtb_debug.txt
cannsim_20260614201022_sim_exe/.soc-version
record.log
```
