#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char** argv) {
    
    char* a[] = {   "./bin/DCR",
		    "--original_task_name=ALGOriginalTask",
		    "--is_scheduler_node=false", 
		    "--decomposer_name=ALGDecomposer",
		    "--result_name=ALGResult",
		    "--computation_task_name=ALGComputationTask",
		    "--granulity_name=ALGGranularity",
		    "--job_name=ALGJob",
		    "--task_file_name=./GPR/ALG_GPR.xml",
			"--granulity_file_name=./GRA/granulity.xml",
		    "--log_file_path=./log/cn",
            "--strategy=3",
			"--node_type_file_name=node_type.xml",
			"--task_name=PlainTask",
			"--xmllog_name=CrackXMLLog"};
	execv("./bin/DCR", a);
	getchar();
	printf("%s", strerror(errno));
}
