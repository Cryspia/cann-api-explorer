# AddHalf -- simulation result

- Status: **passed**
- Build SOC: `Ascend950PR_9599`  Run SOC: `Ascend950`
- PASS marker: `ADD_HALF SIMULATION PASSED`
- Instruction count: 336
- Execution time (ns): 2072.73

## record log (tail)
```
INFO:root:CannProf successfully finished kernel profiling, reports are ready for review
INFO:root:====================
INFO:root:Report INFO
INFO:root:Analysis type: hotspots
INFO:root:Output folder: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/add_half/report/cannsim_20260611184313_sim_exe/report/results_20260612_024340906894
INFO:root:Generated Architecture Diagrams: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/add_half/report/cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports
INFO:root:Chrome Tracing reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/add_half/report/cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports
INFO:root:Critical path reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/add_half/report/cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports
INFO:root:Issue Queue reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/add_half/report/cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports
INFO:root:Scalar IPC dynamic reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/add_half/report/cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports
INFO:root:Vector IPC dynamic reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/add_half/report/cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports
INFO:root:Instruction Cache reports: /home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/add_half/report/cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports
INFO:root:====================
INFO:root:Functions INFO
INFO:root:Kernel name: kernel
INFO:root:Total number of instructions: 0
INFO:root:Total number of instruction executions: 336
INFO:root:Execution time (ns): 2072.73
INFO:root:====================
[2026-06-11 18:43:41] [INFO] Report generation completed successfully.
+==========================================================================+
│ (Report) Cannsim auto generate report successed!                         │
+==========================================================================+
Cannsim auto generate report saved at:
 └─/home/spark/projects/cann-simulator/cann-api-explorer/examples/ascendc/vector/add_half/report/cannsim_20260611184313_sim_exe/report
```

## Report artifacts
```
cannsim_20260611184313_sim_exe/cannsim.log
cannsim_20260611184313_sim_exe/instr.bin
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_operator.svg
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports/aicore_memory_sim_view_bandwidth_per_request.svg
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports/aicore_memory_sim_view_number_of_requests.svg
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports/aicore_utilization.json
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports/core_0_critical_path_report_0.json
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports/core_0_icache_report.html
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports/core_0_scalar_ipc_dynamic_report.html
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports/core_0_tracing_report_0.json
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/kernel_0_reports/instruction_duration_distribution.html
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/perf_log.dump
cannsim_20260611184313_sim_exe/report/results_20260612_024340906894/runner.py.log
cannsim_20260611184313_sim_exe/route_table.txt
cannsim_20260611184313_sim_exe/rtb_debug.txt
cannsim_20260611184313_sim_exe/.soc-version
record.log
```
