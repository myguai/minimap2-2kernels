
def process_file(input_file, output_tmp_files, search_string):
    with open(input_file, 'r') as f:
        lines = f.readlines()
    middle_lines = [line for line in lines if search_string in line]
    print("filter success done")
    with open(output_tmp_files, 'w') as f:
        for index,line in enumerate(middle_lines):
            if(len(line)==255 and index < 2560):
                filtered_line= line[16:].replace(',',' ')
                #print(filtered_line)
                f.write(filtered_line)


search_string = "[sjl-debug]"  
input_file = "../aln_1000x20000bp_cpu.sam" 
output_tmp_files = "out_tmp.txt"
process_file(input_file,output_tmp_files,search_string)
