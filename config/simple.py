import m5
from m5.objects import *

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

#memory
system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('8192MB')]

#CPU
system.cpu = MinorCPU()
#system.cpu = TimingSimpleCPU()

#bus
system.membus = SystemXBar()

#connect cpu with memory
system.cpu.icache_port = system.membus.slave
system.cpu.dcache_port = system.membus.slave

#other ports
system.cpu.createInterruptController()
#system.cpu.interrupts[0].pio = system.membus.master
#system.cpu.interrupts[0].int_master = system.membus.slave
#system.cpu.interrupts[0].int_slave = system.membus.master

system.system_port = system.membus.slave

#memory controller
system.mem_ctrl = DDR3_1600_8x8()
system.mem_ctrl.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.master

#set the process
process = Process()
#process.cmd = ['tests/test-progs/hello/bin/riscv/linux/hello']
process.cmd = ['tests/test-progs/storage-workload/tpch_q6_riscv/riscv_prefetch']
#process.cmd = ['tests/test-progs/storage-workload/arm_state']
system.cpu.workload = process
system.cpu.createThreads()

#root
root = Root(full_system = False, system = system)
m5.instantiate()
print("Beginning sumulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
		.format(m5.curTick(), exit_event.getCause()))
