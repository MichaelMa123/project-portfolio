
.syntax unified //using unified syntax for ARM and Thumb instructions
.cpu cortex-m4 //tells Assembler to generate instructions for Cortex-M4
.thumb //tells Assembler to use Thumb instruction set
.extern SVC_Handler_Main
.extern PVC_osScheule

.global SVC_Handler
.thumb_func
SVC_Handler:

	TST LR, 4
	ITE EQ
	MRSEQ R0, MSP 
	MRSNE R0, PSP
	B SVC_Handler_Main 
.global PendSV_Handler
.thumb_func

PendSV_Handler:
	CPSID   I
	MRS     R0, PSP
	STMDB	R0!, {R4-R11}
	MSR     PSP, R0

    BL      PVC_osScheule

    MRS     R0, PSP
    LDMIA   R0!, {R4-R11}
    MSR     PSP, R0

    CPSIE   I
    LDR     LR, =0xFFFFFFFD
    BX      LR

    @ Never reaches here
    .size PendSV_Handler, .-PendSV_Handler

.global init_First_Task
.thumb_func
init_First_Task:
    MRS     R0, PSP
    LDMIA   R0!, {R4-R11}
    MSR     PSP, R0

    LDR     LR, =0xFFFFFFFD
    BX      LR
    .size init_First_Task, .-init_First_Task
