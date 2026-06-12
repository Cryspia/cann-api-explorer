# Tan -- simulation result

- Status: **passed**
- Host arch: `x86_64`
- Build SOC: `Ascend950PR_9599`  Run SOC: `Ascend950`
- PASS marker: `TAN SIMULATION PASSED`
- Instruction count: 30424
- Execution time (ns): 5595.15

## record log (tail)
```
INFO:root:CannProf successfully finished kernel profiling, reports are ready for review
INFO:root:====================
INFO:root:Report INFO
INFO:root:Analysis type: hotspots
INFO:root:Output folder: /home/ubuntu/programs/cann-api-explorer/examples/ascendc/highlevel/tan/report/cannsim_20260612035756_sim_exe/report/results_20260612_115932220800
INFO:root:Generated Architecture Diagrams: /home/ubuntu/programs/cann-api-explorer/examples/ascendc/highlevel/tan/report/cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports
INFO:root:Chrome Tracing reports: /home/ubuntu/programs/cann-api-explorer/examples/ascendc/highlevel/tan/report/cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports
INFO:root:Critical path reports: /home/ubuntu/programs/cann-api-explorer/examples/ascendc/highlevel/tan/report/cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports
INFO:root:Issue Queue reports: /home/ubuntu/programs/cann-api-explorer/examples/ascendc/highlevel/tan/report/cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports
INFO:root:Scalar IPC dynamic reports: /home/ubuntu/programs/cann-api-explorer/examples/ascendc/highlevel/tan/report/cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports
INFO:root:Vector IPC dynamic reports: /home/ubuntu/programs/cann-api-explorer/examples/ascendc/highlevel/tan/report/cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports
INFO:root:Instruction Cache reports: /home/ubuntu/programs/cann-api-explorer/examples/ascendc/highlevel/tan/report/cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports
INFO:root:====================
INFO:root:Functions INFO
INFO:root:Kernel name: kernel
INFO:root:Total number of instructions: 0
INFO:root:Total number of instruction executions: 30424
INFO:root:Execution time (ns): 5595.15
INFO:root:====================
[2026-06-12 03:59:34] [INFO] Report generation completed successfully.
+==========================================================================+
│ (Report) Cannsim auto generate report successed!                         │
+==========================================================================+
Cannsim auto generate report saved at:
 └─/home/ubuntu/programs/cann-api-explorer/examples/ascendc/highlevel/tan/report/cannsim_20260612035756_sim_exe/report
```

## Report artifacts
```
cannsim_20260612035756_sim_exe/cannsim.log
cannsim_20260612035756_sim_exe/instr.bin
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_operator.svg
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_request.svg
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/aicore_memory_sim_view_number_of_requests.svg
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/aicore_utilization.json
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_0_critical_path_report_0.json
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_0_icache_report.html
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_0_scalar_ipc_dynamic_report.html
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_0_tracing_report_0.json
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_16_critical_path_report_0.json
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_16_icache_report.html
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_16_scalar_ipc_dynamic_report.html
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_16_tracing_report_0.json
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_17_critical_path_report_0.json
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_17_icache_report.html
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_17_scalar_ipc_dynamic_report.html
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_17_tracing_report_0.json
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_1_critical_path_report_0.json
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_1_icache_report.html
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_1_scalar_ipc_dynamic_report.html
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/core_1_tracing_report_0.json
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/kernel_0_reports/instruction_duration_distribution.html
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/perf_log.dump
cannsim_20260612035756_sim_exe/report/results_20260612_115932220800/runner.py.log
cannsim_20260612035756_sim_exe/route_table.txt
cannsim_20260612035756_sim_exe/rtb_debug.txt
cannsim_20260612035756_sim_exe/.soc-version
record.log
```
