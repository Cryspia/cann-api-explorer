# Max -- simulation result

- Status: **passed**
- Host arch: `x86_64`
- Build SOC: `Ascend950PR_9599`  Run SOC: `Ascend950`
- PASS marker: `MAX SIMULATION PASSED`
- Instruction count: 30904
- Execution time (ns): 6093.94

## record log (tail)
```

[2026-06-12 04:34:31] [INFO] Command executed successfully in 94.80s
[2026-06-12 04:34:31] [INFO] User application executed successfully!
[2026-06-12 04:34:31] [INFO] Collecting <USER_APP PATH>/assets to <OUTPUT PATH>/...
[2026-06-12 04:34:31] [WARN] Log file/directory dirSaveDump.txt not found in /home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/max/build
[2026-06-12 04:34:31] [INFO] Collected assets from /home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/max/build to /home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/max/report/cannsim_20260612043255_sim_exe
+==========================================================================+
│ [SUCCESSED] cannsim record executed successfully! [2026-06-12 04:34:31]  │
+==========================================================================+
See CANNSIM LOG at:
└─/home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/max/report/cannsim_20260612043255_sim_exe/cannsim.log

+==========================================================================+
│ (Report) Cannsim auto generate report...                                 │
+==========================================================================+
[2026-06-12 04:34:31] [INFO] Generating performance report...
[2026-06-12 04:34:31] [INFO] Detected instr.bin, restoring log_ca from binary...
[2026-06-12 04:34:31] [INFO] Executing cannprof report...
[2026-06-12 04:34:31] [INFO] Using cannprof binary: /home/ubuntu/miniforge3/envs/cannsim/cann/cann-9.1.0-beta.1/python/site-packages/cannsim/bin/cannprof
[2026-06-12 04:34:34] [INFO] Report generation completed successfully.
+==========================================================================+
│ (Report) Cannsim auto generate report successed!                         │
+==========================================================================+
Cannsim auto generate report saved at:
 └─/home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/max/report/cannsim_20260612043255_sim_exe/report
```

## Report artifacts
```
cannsim_20260612043255_sim_exe/cannsim.log
cannsim_20260612043255_sim_exe/instr.bin
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_operator.svg
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_request.svg
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/aicore_memory_sim_view_number_of_requests.svg
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/aicore_utilization.json
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_0_critical_path_report_0.json
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_0_icache_report.html
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_0_scalar_ipc_dynamic_report.html
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_0_tracing_report_0.json
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_16_critical_path_report_0.json
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_16_icache_report.html
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_16_scalar_ipc_dynamic_report.html
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_16_tracing_report_0.json
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_17_critical_path_report_0.json
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_17_icache_report.html
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_17_scalar_ipc_dynamic_report.html
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_17_tracing_report_0.json
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_1_critical_path_report_0.json
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_1_icache_report.html
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_1_scalar_ipc_dynamic_report.html
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/core_1_tracing_report_0.json
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/kernel_0_reports/instruction_duration_distribution.html
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/perf_log.dump
cannsim_20260612043255_sim_exe/report/results_20260612_123431921420/runner.py.log
cannsim_20260612043255_sim_exe/route_table.txt
cannsim_20260612043255_sim_exe/rtb_debug.txt
cannsim_20260612043255_sim_exe/.soc-version
record.log
```
