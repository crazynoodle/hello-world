#include "cmd_manager.h"

int main(int argc, char** argv) {
	
	CMDManager *cmdManager = new CMDManager();
	cmdManager->startWork();
	
	for (int i = 0; i < 1; i ++) {

		//cmdManager->createTask("../../GPR/lbj_0801_gpr.xml");
		cmdManager->createTask("../../GPR/LBJ_task_GPR.xml");
		//cmdManager->createTask("../../GPR/Ntlm_730_1.xml");
		sleep(1);
		/*
		cmdManager->createTask("../../GPR/ALG_GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/cjh_20150714_3_C_0.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_0.GPR.xml");
		sleep(1);*/
		/*
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_1.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_2.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_3.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_4.GPR.xml");
		sleep(1);

		cmdManager->createTask("../../GPR/b3335_20150729_2_C_5.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_6.GPR.xml");
		sleep(1);
		
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_7.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_8.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_9.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_10.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_11.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_12.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_13.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_14.GPR.xml");
		sleep(1);
		cmdManager->createTask("../../GPR/b3335_20150729_2_C_15.GPR.xml");
		sleep(1);*/

	}
	delete cmdManager;

}
