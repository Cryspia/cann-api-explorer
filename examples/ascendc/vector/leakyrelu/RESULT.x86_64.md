# LeakyRelu -- simulation result

- Status: **passed**
- Host arch: `x86_64`
- Build SOC: `Ascend950PR_9599`  Run SOC: `Ascend950`
- PASS marker: `LEAKYRELU SIMULATION PASSED`
- Instruction count: 286
- Execution time (ns): 1907.27

## record log (tail)
```
INFO:root:Total number of instructions: 0
INFO:root:Total number of instruction executions: 286
INFO:root:Execution time (ns): 1907.27
INFO:root:====================
[2026-06-15 05:16:26] [WARN] Log file/directory dirSaveDump.txt not found in /home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/leakyrelu/build
[2026-06-15 05:16:26] [INFO] Collected assets from /home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/leakyrelu/build to /home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/leakyrelu/report/cannsim_20260615051534_sim_exe
+==========================================================================+
│ [SUCCESSED] cannsim record executed successfully! [2026-06-15 05:16:26]  │
+==========================================================================+
See CANNSIM LOG at:
└─/home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/leakyrelu/report/cannsim_20260615051534_sim_exe/cannsim.log

+==========================================================================+
│ (Report) Cannsim auto generate report...                                 │
+==========================================================================+
[2026-06-15 05:16:26] [INFO] Generating performance report...
[2026-06-15 05:16:26] [INFO] Detected instr.bin, restoring log_ca from binary...
[2026-06-15 05:16:26] [INFO] Executing cannprof report...
[2026-06-15 05:16:26] [INFO] Using cannprof binary: /home/ubuntu/miniforge3/envs/cannsim/cann/cann-9.1.0-beta.1/python/site-packages/cannsim/bin/cannprof
[2026-06-15 05:16:27] [INFO] Report generation completed successfully.
+==========================================================================+
│ (Report) Cannsim auto generate report successed!                         │
+==========================================================================+
Cannsim auto generate report saved at:
 └─/home/ubuntu/programs/cann-api-explorer/examples/ascendc/vector/leakyrelu/report/cannsim_20260615051534_sim_exe/report
```

## Report artifacts
```
cannsim_20260615051534_sim_exe/cannsim.log
cannsim_20260615051534_sim_exe/instr.bin
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_operator.svg
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_request.svg
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/kernel_0_reports/aicore_memory_sim_view_number_of_requests.svg
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/kernel_0_reports/aicore_utilization.json
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/kernel_0_reports/core_0_critical_path_report_0.json
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/kernel_0_reports/core_0_icache_report.html
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/kernel_0_reports/core_0_scalar_ipc_dynamic_report.html
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/kernel_0_reports/core_0_tracing_report_0.json
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/kernel_0_reports/instruction_duration_distribution.html
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/perf_log.dump
cannsim_20260615051534_sim_exe/report/results_20260615_131627096523/runner.py.log
cannsim_20260615051534_sim_exe/route_table.txt
cannsim_20260615051534_sim_exe/rtb_debug.txt
cannsim_20260615051534_sim_exe/.soc-version
record.log
```
