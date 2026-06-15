# Mul -- simulation result

- Status: **passed**
- Host arch: `aarch64`
- Build SOC: `Ascend950PR_9599`  Run SOC: `Ascend950`
- PASS marker: `MUL SIMULATION PASSED`
- Instruction count: 30900
- Execution time (ns): 7072.73

## record log (tail)
```
INFO:root:Total number of instructions: 0
INFO:root:Total number of instruction executions: 30900
INFO:root:Execution time (ns): 7072.73
INFO:root:====================
[2026-06-14 19:56:05] [WARN] Log file/directory dirSaveDump.txt not found in /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/mul/build
[2026-06-14 19:56:05] [INFO] Collected assets from /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/mul/build to /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/mul/report/cannsim_20260614195516_sim_exe
+==========================================================================+
│ [SUCCESSED] cannsim record executed successfully! [2026-06-14 19:56:05]  │
+==========================================================================+
See CANNSIM LOG at:
└─/home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/mul/report/cannsim_20260614195516_sim_exe/cannsim.log

+==========================================================================+
│ (Report) Cannsim auto generate report...                                 │
+==========================================================================+
[2026-06-14 19:56:05] [INFO] Generating performance report...
[2026-06-14 19:56:05] [INFO] Detected instr.bin, restoring log_ca from binary...
[2026-06-14 19:56:05] [INFO] Executing cannprof report...
[2026-06-14 19:56:05] [INFO] Using cannprof binary: /home/spark/miniforge3/envs/cannsim/cann/cann-9.1.0-beta.1/python/site-packages/cannsim/bin/cannprof
[2026-06-14 19:56:08] [INFO] Report generation completed successfully.
+==========================================================================+
│ (Report) Cannsim auto generate report successed!                         │
+==========================================================================+
Cannsim auto generate report saved at:
 └─/home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/mul/report/cannsim_20260614195516_sim_exe/report
```

## Report artifacts
```
cannsim_20260614195516_sim_exe/cannsim.log
cannsim_20260614195516_sim_exe/instr.bin
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_operator.svg
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_request.svg
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/aicore_memory_sim_view_number_of_requests.svg
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/aicore_utilization.json
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_0_critical_path_report_0.json
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_0_icache_report.html
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_0_scalar_ipc_dynamic_report.html
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_0_tracing_report_0.json
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_16_critical_path_report_0.json
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_16_icache_report.html
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_16_scalar_ipc_dynamic_report.html
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_16_tracing_report_0.json
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_17_critical_path_report_0.json
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_17_icache_report.html
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_17_scalar_ipc_dynamic_report.html
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_17_tracing_report_0.json
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_1_critical_path_report_0.json
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_1_icache_report.html
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_1_scalar_ipc_dynamic_report.html
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/core_1_tracing_report_0.json
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/kernel_0_reports/instruction_duration_distribution.html
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/perf_log.dump
cannsim_20260614195516_sim_exe/report/results_20260615_035606147705/runner.py.log
cannsim_20260614195516_sim_exe/route_table.txt
cannsim_20260614195516_sim_exe/rtb_debug.txt
cannsim_20260614195516_sim_exe/.soc-version
record.log
```
