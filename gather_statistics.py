#!/usr/bin/env python3
import os
import sys
import matplotlib.pyplot as plt

cwd = os.getcwd()
test_case_path = "scratch/2nodes_simple"
test_script_path = "scratch/ub-quick-example"
network_attribute_file = os.path.join(test_case_path, "network_attribute.txt")
traffic_file_path = os.path.join(test_case_path, "traffic.csv")
output_throughput_path = os.path.join(test_case_path, "output/task_statistics.csv")
output_path = "statistics.txt"

length = 100
end_mpki = 50
trials = 20

traffic_file_content = '''taskId,sourceNode,destNode,dataSize(Byte),opType,priority,delay,phaseId,dependOnPhases
0,0,1,{},MEM_STORE,7,10ns,0,

'''

def measure_throughput_no_internal_traffic(show_graph=True, store_path=None):
    latency = []
    throughput = []
    for i in range(1, length+1):
        with open(traffic_file_path, "w") as traffic_file_hd:
            traffic_file_hd.write(traffic_file_content.format(i * 10000))

        os.system("./ns3 run \"" + test_script_path + " " + test_case_path +"\"" )

        print("Simulation of " + str(i * 10000) + " bytes finished")

        with open(output_throughput_path, "r") as output_throughput_path_hd:
            _ = output_throughput_path_hd.readline()
            line = output_throughput_path_hd.readline()
            line = line.split(',')
            latency.append(float(line[10])-float(line[9]))
            throughput.append(float(line[13]))

    print("latency:")
    print(latency)

    print("throughput:")
    print(throughput)

    with open(output_path, 'w') as output_file_hd:
        output_file_hd.write(str(latency))
        output_file_hd.write('\n')
        output_file_hd.write(str(throughput))

    x_axis = [i * 10000 for i in range(1, length+1)]

    fig, ax1 = plt.subplots()
    ax2 = ax1.twinx()
    ax1.plot(x_axis, latency, "b-")
    ax2.plot(x_axis, throughput, "y-")
    ax1.set_xlabel("Total transferred bytes")
    ax1.set_ylabel("Latency (us)")
    ax2.set_ylabel("Throughput (Gbps)")

    if show_graph:
        plt.show()
    if store_path is not None:
        store_path = os.path.join(cwd, store_path)
        plt.savefig(store_path)
        

def calculate_real_throughput():
    import numpy as np

    with open(output_path, 'r') as output_file_hd:
        latency = output_file_hd.readline()[1:-1]
        if latency[-1] == ']':
            latency = latency[:-1]
        latency = latency.split(',')
        latency = list(map(lambda x: float(x), latency))

    y_variable = list(map(lambda x: x * 10000 * 8 * 1000 * 1000 / 1000000000, range(1, length+1)))

    m, b = np.polyfit(latency, y_variable, 1)
    print(m)
    return m

def measure_throughput_nit_with_different_packet_size():
    end_store_length = 7
    throughput = []
    overhead_enabled = True

    for i in range(end_store_length+1):
        network_attribute_line = "default ns3::UbLdstThread::StoreRequestLength \"{}\"\n"
        with open(network_attribute_file, 'r') as network_attribute_hd:
            lines = network_attribute_hd.readlines()
        id_line = -1
        for idx, line in enumerate(lines):
            if line[:45] == "default ns3::UbLdstThread::StoreRequestLength":
                id_line = idx
                print("Found required attribute on line {}, modified content".format(idx))
        if id_line == -1:
            lines.append(network_attribute_line.format(i))
        else:
            lines[id_line] = network_attribute_line.format(i)
        
        with open(network_attribute_file, 'w') as network_attribute_hd:
            network_attribute_hd.writelines(lines)

        output_fig_name = "throughput_nit_0ns_{}bytes.png".format(64 * pow(2, i))
        if overhead_enabled:
            output_fig_name = "throughput_nit_100ns_{}bytes.png".format(64 * pow(2, i))
        measure_throughput_no_internal_traffic(show_graph=False, store_path="output_graphs/no_internal_traffic/"+output_fig_name)
        throughput.append(calculate_real_throughput())
    
    output_data_name = "throughput_nit_0ns.txt"
    if overhead_enabled:
        output_data_name = "throughput_nit_100ns.txt"
    with open(output_data_name, 'w') as output_hd:
        output_hd.write(str(throughput))
    
    x_axis = [64 * pow(2, i) for i in range(0,end_store_length+1)]
    x_index = list(range(len(x_axis)))
    fig, ax = plt.subplots()
    ax.plot(x_axis, throughput)
    ax.set_xscale('log', base=2)
    ax.set_xticks(x_axis, x_axis)
    ax.set_xlabel("Packet size in bytes")
    ax.set_ylabel("Throughput in Gbps")
    plt.show()
    

def generate_throughput_vs_packet_size_curve():
    import numpy as np

    end_store_length = 7
    overhead_enabled = False
    output_data_name = "throughput_nit_0ns.txt"
    if overhead_enabled:
        output_data_name = "throughput_nit_100ns.txt"
    with open(output_data_name, 'r') as output_hd:
        line = output_hd.readline()
        line = line[1:-2]
        line = line.split(',')
        line = list(map(lambda x: x[11:], line))
        line = list(map(lambda x: float(''.join(list(filter(lambda y: y.isdigit() or y == '.', x)))), line))

    x_axis = [64 * pow(2, i) for i in range(0,end_store_length+1)]
    x_index = list(range(len(x_axis)))
    fig, ax = plt.subplots()
    ax.plot(x_axis, line)
    ax.set_xscale('log', base=2)
    ax.set_xticks(x_axis, x_axis)
    ax.set_xlabel("Packet size in bytes")
    ax.set_ylabel("Throughput in Gbps")
    plt.show()


def measure_throughput_with_internal_traffic():
    total_bytes = 100000

    throughput = []

    MPKI_definition_path = "src/unified-bus/model/traffic-macro.h"
    macro_content = '''
#define LLC_MISS_PER_KILO_INSTRUCTION {}
#define LLC_MISS_PROBABILITY 10000 / LLC_MISS_PER_KILO_INSTRUCTION
'''
    output_statistics_path = "internal.txt"
    debug_log_file = "internal_debug.txt"

    debug_log_hd = open(debug_log_file, 'w')

    for mpki in range(1, end_mpki+1):

        debug_log_hd.write("MPKI="+str(mpki * 0.05)+":\n")
        acc_throughput = 0.0

        with open(MPKI_definition_path, 'w') as mpki_file_hd:
            mpki_file_hd.write(macro_content.format(mpki * 0.05))

        for i in range(trials):
            debug_log_hd.write("Trial "+str(i)+": ")
            with open(traffic_file_path, "w") as traffic_file_hd:
                traffic_file_hd.write(traffic_file_content.format(total_bytes))

            run_command = "./ns3 run \"" + test_script_path + " " + test_case_path +"\""
            run_command = "NS_GLOBAL_VALUE=\"RngRun={}\" ".format(i) + run_command
            os.system(run_command)

            with open(output_throughput_path, "r") as output_throughput_path_hd:
                _ = output_throughput_path_hd.readline()
                line = output_throughput_path_hd.readline()
                line = line.split(',')
                acc_throughput += float(line[13])
                content = line[13] + '\n'
                debug_log_hd.write(content)
        
        acc_throughput /= trials
        throughput.append(acc_throughput)

        print("----------------------------------")
        print("MPKI = " + str(mpki * 0.05) + " simulated")
        print("----------------------------------")
        
    with open(output_statistics_path, 'w') as output_statistics_hd:
        output_statistics_hd.write(str(throughput))
    
    x_axis = list(range(1, end_mpki+1))
    plt.plot(x_axis, throughput)
    plt.show()
    debug_log_hd.close()






def measure_baseline_throughput():
    # Comment the SIM_INTERNAL_HBM macro in control-macro.h out first

    trials = 100
    total_bytes = 200000

    output_statistics_path = "baseline.txt"
    debug_log_file = "baseline_debug.txt"

    macro_content = '''
#define LLC_MISS_PER_KILO_INSTRUCTION {}
#define LLC_MISS_PROBABILITY 10000 / LLC_MISS_PER_KILO_INSTRUCTION
'''
    debug_log_hd = open(debug_log_file, 'w')

    acc_throughput = 0.0

    for i in range(trials):
        debug_log_hd.write("Trial "+str(i)+": ")
        with open(traffic_file_path, "w") as traffic_file_hd:
            traffic_file_hd.write(traffic_file_content.format(total_bytes))

        run_command = "./ns3 run \"" + test_script_path + " " + test_case_path +"\""
        run_command = "NS_GLOBAL_VALUE=\"RngRun={}\" ".format(i) + run_command
        os.system(run_command)

        with open(output_throughput_path, "r") as output_throughput_path_hd:
            _ = output_throughput_path_hd.readline()
            line = output_throughput_path_hd.readline()
            line = line.split(',')
            acc_throughput += float(line[13])
            content = line[13] + '\n'
            debug_log_hd.write(content)
    
    acc_throughput /= trials
        
    with open(output_statistics_path, 'w') as output_statistics_hd:
        output_statistics_hd.write(str(acc_throughput))
    
    print(acc_throughput)

def make_output_graph():
    baseline_statistics_path = "baseline.txt"
    output_statistics_path = "data.txt"
    
    baseline_throughput = 0
    with open(baseline_statistics_path, 'r') as baseline_statistics_hd:
        baseline_throughput = float(baseline_statistics_hd.readline())
    
    x_axis = list(map(lambda x: x / 100.0, range(0, end_mpki+1)))
    y_axis_1 = None

    with open(output_statistics_path, 'r') as output_statistics_hd:
        line = output_statistics_hd.readline()
        line = line[1:-2]
        line = line.split(',')
        y_axis_1 = list(map(lambda x: float(x), line))

    y_axis_1.insert(0, baseline_throughput)
    y_axis_2 = list(map(lambda x: baseline_throughput / x, y_axis_1))
    y_axis_2 = list(map(lambda x: 100 * (x-1), y_axis_2))

    fig, ax1 = plt.subplots()
    ax2 = ax1.twinx()

    ax1.plot(x_axis, y_axis_1, 'b-')
    ax2.plot(x_axis, y_axis_2, 'y-')
    ax1.set_xlabel("MPKI (%)")
    ax1.set_ylabel("Throughput (Gbps)")
    ax2.set_ylabel("Latency Increase (%)")
    plt.show()

def urgent_calculation():
    output_statistics_path = "internal_debug_1.txt"
    
    acc = 0
    counter = 0
    throughput = []
    with open(output_statistics_path, 'r') as output_statistics_hd:
        for line in output_statistics_hd:
            if len(line) in [0, 1]:
                continue
            if line[0] == 'M':
                continue
            text = line.split(':')[-1]
            text = text[:-1]
            print(text)
            throughput_temp = float(text)
            acc += throughput_temp
            counter = counter + 1
            if counter == trials:
                print("Test")
                counter = 0
                throughput.append(acc / 100)
                acc = 0
    
    print(throughput)
    with open("data.txt", 'w') as output_statistics_hd:
        output_statistics_hd.write(str(throughput))

if __name__ == "__main__":
    # measure_throughput_no_internal_traffic()
    # calculate_real_throughput()
    # measure_throughput_nit_with_different_packet_size()
    # generate_throughput_vs_packet_size_curve()
    measure_throughput_with_internal_traffic()
    # measure_baseline_throughput()
    # make_output_graph()
    #urgent_calculation()
    pass
