#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define INST_MEM_SIZE 1024
#define DATA_MEM_SIZE 1024
#define CR  0b0110011
#define CI  0b0010011
#define CU  0b0110111
#define CLI 0b0000011 //L mean memory load
#define CS  0b0100011
#define CSB 0b1100011
#define CUJ 0b1101111
#define CJR 0b1100111

int32_t sext12_to_32(uint32_t num){
    return (0xfffff000|num)*(num >= 2048) +(0x00000fff&num)*(num < 2048);
}

void reg_dump(uint32_t registr[],uint16_t pc){
    printf("PC = 0x%08x;\n",pc);
    for(int blah = 0;blah < 32;blah++){
        printf("R[%i] = 0x%08x;\n", blah, registr[blah]);
    }
}
void invalid_instruction(int instruction,uint32_t registr[],uint16_t pc){
    printf("Instruction Not Implemented: 0x%08x\n",instruction);
    reg_dump(registr,pc);
    exit(1);
}
void check_M_address(int imme, int instruction,uint32_t registr[],uint16_t pc){
    if (imme >= DATA_MEM_SIZE + INST_MEM_SIZE || imme < 0){
        printf("Illegal Operation: 0x%08x\n",instruction);
        reg_dump(registr,pc);
        exit(1);
    }
}


int main(int argc,char *argv[]) {
    uint16_t pc = 0;
    uint32_t r[32] = {0}; //registers
    union instruction{
        uint32_t whole;

        struct{
        unsigned int op: 7;
        unsigned int rd : 5;
        unsigned int func3 : 3;
        unsigned int rs1 : 5;
        unsigned int rs2 : 5;
        unsigned int func7 : 7;
        }R;

        struct type_I{
            int moo:20;
            unsigned int imm:12;
        }I;

        struct type_S{
            int moo: 7;
            unsigned int imm1: 5;
            int mooo: 13;
            unsigned int imm32: 7; 
        }S;

        struct type_SB{
            int moo: 7;
            unsigned int imm2048: 1;
            unsigned int imm2: 4;
            int mooo: 13;
            unsigned int imm32: 6;
            unsigned int imm4096: 1;
        }SB;

        struct type_U{
            int moo: 12;
            unsigned int imm: 20;
        }U;

        struct type_UJ{
            int moo: 12;
            unsigned int imm12: 8;
            unsigned int imm11: 1;
            unsigned int imm1:10;
            unsigned int imm20:1;
        }UJ;
    }i;
    uint8_t d[DATA_MEM_SIZE + INST_MEM_SIZE];
    uint32_t imme;

    

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
       printf("Error loading file");exit(1);
    }else if (!fread(&d[0], DATA_MEM_SIZE + INST_MEM_SIZE,1, file)){
        printf("Not enough data\n");exit(1);
    }else if(fread(&d[0], 1, 1, file)){
        printf("Memory file longer than expected\n");exit(1);
    }else{fclose(file);}

    //char step[100] = "bla";
    while(pc < INST_MEM_SIZE && pc >= 0){
        r[0] = 0;
        i.whole = d[pc] + d[pc+1]*0x100+ d[pc+2]*0x10000 + d[pc+3]*0x1000000;
        //printf("%i |%08x||",pc,i.whole);

        //if (pc%4 != 0){
        //    printf("Weird pc %i",pc);
        //    exit(1);
        //}
        
        
        switch (i.R.op) {
            case CR:
                switch (i.R.func3){
                    case 0b000:
                        if (i.R.func7 == 0){
                            r[i.R.rd]=r[i.R.rs1] + r[i.R.rs2];//add
                            break;
                            //sprintf(step,"add, register %i is now %i",i.R.rd,r[i.R.rd]);
                        }else if(i.R.func7 == 0b100000){
                            r[i.R.rd]=r[i.R.rs1] - r[i.R.rs2];
                            break;
                            //sprintf(step,"minus, register %i is now %i",i.R.rd,r[i.R.rd]);
                        }

                    case 0b100:
                        if(i.R.func7 == 0){
                            r[i.R.rd] = r[i.R.rs1] ^ r[i.R.rs2];//xor
                            break;
                            //sprintf(step,"xor, register %i is now %i",i.R.rd,r[i.R.rd]);
                        }
                    case 0b110:
                        if(i.R.func7 == 0){
                            r[i.R.rd] = r[i.R.rs1] | r[i.R.rs2];
                            break;//or
                            //sprintf(step,"or, register %i is now %i",i.R.rd,r[i.R.rd]);
                        }

                    case 0b111:
                        if(i.R.func7 == 0){
                            r[i.R.rd] = r[i.R.rs1] & r[i.R.rs2];//
                            break;
                            //sprintf(step,"and, register %i is now %i",i.R.rd,r[i.R.rd]);
                        }
                    
                    case 0b001:
                        if(i.R.func7 == 0){
                            r[i.R.rd] = r[i.R.rs1] << r[i.R.rs2];
                            break;
                            //sprintf(step,"shift<<, register %i is now %i",i.R.rd,r[i.R.rd]);
                        }

                    case 0b101:
                        if(i.R.func7 == 0){
                            r[i.R.rd] = r[i.R.rs1] >> r[i.R.rs2];
                            break;
                            //sprintf(step,"shift>>, register %i is now %i",i.R.rd,r[i.R.rd]);
                        }else if(i.R.func7 == 0b0100000){
                            r[i.R.rd] = (r[i.R.rs1] >> (r[i.R.rs2]% 32 )) | (r[i.R.rs1] << (32 - (r[i.R.rs2]% 32 )));
                            break;
                            //sprintf(step,"rotate right, register %i is now %i",i.R.rd,r[i.R.rd]);
                        }
                    
                    case 0b010:
                        if(i.R.func7 == 0){
                            r[i.R.rd] =((signed)r[i.R.rs1] < (signed)r[i.R.rs2]);
                            break;
                            //sprintf(step,"signed less than, register %i is now %i, r%i vs r%i ",i.R.rd,r[i.R.rd],i.R.rs1,i.R.rs2);
                        }
                    break;case 0b011:
                        if(i.R.func7 == 0){
                            r[i.R.rd]=(r[i.R.rs1] < r[i.R.rs2]);
                            break;
                             //sprintf(step,"unsigned less than, register %i is now %i, r%i vs r%i ",i.R.rd,r[i.R.rd],i.R.rs1,i.R.rs2);
                        }

                    default:
                        invalid_instruction(i.whole,r,pc);
                    break;
                }

            break;case CI:
                imme = sext12_to_32(i.I.imm);
                switch (i.R.func3){
                    case 0b0:
                        r[i.R.rd]=r[i.R.rs1] + imme;//addi
                         //sprintf(step,"addi, register %i is now %i, r%i and imm:%i ",i.R.rd,r[i.R.rd],i.R.rs1,sext12_to_32(i.I.imm));

                    break;case 0b100:
                        r[i.R.rd] = r[i.R.rs1] ^ imme;
                        //sprintf(step,"xori, register %i is now %i, r%i and imm:%i ",i.R.rd,r[i.R.rd],i.R.rs1,sext12_to_32(i.I.imm));

                    break;case 0b110:
                        r[i.R.rd] = r[i.R.rs1] | imme;
                        //sprintf(step,"ori, register %i is now %i, r%i and imm:%i ",i.R.rd,r[i.R.rd],i.R.rs1,sext12_to_32(i.I.imm));

                    break;case 0b111:
                        r[i.R.rd] = r[i.R.rs1] & imme;
                        //sprintf(step,"andi, register %i is now %i, r%i and imm:%i ",i.R.rd,r[i.R.rd],i.R.rs1,sext12_to_32(i.I.imm));

                    break;case 0b010:
                        r[i.R.rd]=((signed)r[i.R.rs1] < (signed)imme);
                        //sprintf(step,"les than im sig, register %i is now %i, r%i and imm:%i ",i.R.rd,r[i.R.rd],i.R.rs1,sext12_to_32(i.I.imm));

                    break;case 0b011:
                        r[i.R.rd] = (r[i.R.rs1] < imme);
                        //sprintf(step,"les than unsig, register %i is now %i, r%i and imm:%i ",i.R.rd,r[i.R.rd],i.R.rs1,sext12_to_32(i.I.imm));

                    break;default:
                        invalid_instruction(i.whole,r,pc);
                    break;
                }

            break;case CU://lui
                r[i.R.rd] = i.U.imm;
                r[i.R.rd] = (r[i.R.rd] << 12);
                //sprintf(step,"lui, register %i is now %i, imm:%i ",i.R.rd,r[i.R.rd],i.U.imm);
                    
            break;case CLI:
                imme = r[i.R.rs1] + sext12_to_32(i.I.imm);
                //sprintf(step,"virtual load routine at hex:%x",imme);
                //printf("load attemp at %i",imme);
                
                switch (imme){ //check for virtual routine
                    case 0x0812:
                        r[i.R.rd] = getchar();
                        
                    break;case 0x0816:
                        r[0]=scanf("%d", &r[i.R.rd]);
                        

                    break;default:

                        switch (i.R.func3){
                            case 0:
                                check_M_address(imme,i.whole,r,pc);
                                r[i.R.rd] = (d[imme]>= 128)*(d[imme]|0xffffff00)+(d[imme]<128)*(d[imme]);
                                
                                //sprintf(step,"load byte at hex:%x,value %i",imme,r[i.R.rd]);
                            break;case 0b001:
                                check_M_address(imme + 1,i.whole,r,pc);
                                r[i.R.rd] = d[imme] *256 + d[imme+1];
                                r[i.R.rd] = (r[i.R.rd]|0xffff0000)*((d[imme])>=128)+((d[imme])<128)*(r[i.R.rd]);
                                //sprintf(step,"load half at hex:%x,value %i",imme,r[i.R.rd]);

                            break;case 0b010:
                                check_M_address(imme+3,i.whole,r,pc);
                                r[i.R.rd] =d[imme]*16777216 + d[imme+1]*65536+d[imme+2] *256 + d[imme+3];
                                //sprintf(step,"load w at hex:%x,value %i",imme,r[i.R.rd]);
                            
                            break;case 0b100:
                                check_M_address(imme,i.whole,r,pc);
                                r[i.R.rd] = d[imme];
                                //sprintf(step,"load byte unsi at hex:%x,value %i",imme,r[i.R.rd]);

                            break;case 0b101:
                                check_M_address(imme+1,i.whole,r,pc);
                                r[i.R.rd] = d[imme]*256 + d[imme+1];
                                //sprintf(step,"load half unsi at hex:%x,value %i",imme,r[i.R.rd]);

                            break;default:
                                invalid_instruction(i.whole,r,pc);
                            break;
                        }
                    break;
                    }

            break;case CS:
                imme = r[i.R.rs1] + sext12_to_32(i.S.imm1 + i.S.imm32*32);
                //sprintf(step,"virtual routine store at hex:%x",imme);
                switch (imme){
                    
                    case 0x080c:
                        printf("CPU Halt Requested\n");
                        exit(0);

                    break;case 0x0830:
                        r[28] = 0xb700;

                    break;case 0x0800:
                        printf("%c",(char)(r[i.R.rs2]&0x000000ff));

                    break;case 0x0828:
                        check_M_address(r[i.R.rs2],i.whole,r,pc);
                        printf("%x",(d[r[i.R.rs2]]));

                    break;case 0x0804:
                        printf("%i",r[i.R.rs2]);

                    break;case 0x0808:
                        printf("%x",r[i.R.rs2]);

                    break;case 0x0820:
                        printf("%x",pc);
                
                    break;case 0x0824:
                        reg_dump(r,pc);

                    break;default:
                        switch (i.R.func3){
                            case 0:
                                check_M_address(imme,i.whole,r,pc);
                                d[imme]= r[i.R.rs2] & 0x000000ff;
                                //sprintf(step,"store byte at hex:%x,value %i",imme,d[imme]);


                            break;case 0b001:
                                check_M_address(imme+1,i.whole,r,pc);
                                d[imme] = ((r[i.R.rs2] & 0x0000ff00)>>8);
                                d[imme + 1] = r[i.R.rs2] & 0x000000ff;
                                //sprintf(step,"store half unsi at hex:%x,value %i and +1:%i",imme,d[imme],d[imme+1]);

                            break;case 0b010:
                                check_M_address(imme+3,i.whole,r,pc);
                                d[imme] = ((r[i.R.rs2] &   0xff000000)>>24);
                                d[imme+1] = ((r[i.R.rs2] & 0x00ff0000)>>16);
                                d[imme+2] = ((r[i.R.rs2] & 0x0000ff00)>>8);
                                d[imme+3] = ((r[i.R.rs2] & 0x000000ff));
                                //sprintf(step,"store w at hex:%x,value %i and +1:%i +2:%i +3:%i",imme,d[imme],d[imme+1],d[imme+2],d[imme+3] );

                            break;default:
                                invalid_instruction(i.whole,r,pc);
                            break;
                        }
                    break;
                      
                }


            break;case CSB:
                imme = (sext12_to_32(i.SB.imm2048*1024 + i.SB.imm2 + i.SB.imm32*16 + i.SB.imm4096*2048)<<1) - 4;
                switch (i.R.func3){
                    case 0:
                        pc+= (r[i.R.rs1] == r[i.R.rs2])*imme;
                            //sprintf(step,"bran equal imm:%i,new pc is %i,r%i vs r%i",immeSB,pc,i.R.rs1,i.R.rs2);

                    break;case 1:
                        pc+= (r[i.R.rs1] != r[i.R.rs2])*imme;
                            //sprintf(step,"bran not equal imm:%i,new pc is %i,r%i vs r%i",immeSB,pc,i.R.rs1,i.R.rs2);
                            
                    break;case 0b100:
                        pc +=((signed)r[i.R.rs1] < (signed)r[i.R.rs2])*imme;
                            //sprintf(step,"bran less than signed imm:%i,new pc is %i,r%i vs r%i",immeSB,pc,i.R.rs1,i.R.rs2);
            
                    break;case 0b110:
                        pc+= imme*(r[i.R.rs1] < r[i.R.rs2]);
                            //sprintf(step,"bran less than unsigned imm:%i,new pc is %i,r%i vs r%i",immeSB,pc,i.R.rs1,i.R.rs2);
                        

                    break;case 0b101:
                        pc+=((signed)r[i.R.rs1] >= (signed)r[i.R.rs2])*imme;
                            //sprintf(step,"bran big or equal signed imm:%i,new pc is %i,r%i vs r%i",immeSB,pc,i.R.rs1,i.R.rs2);
                        
                    break;case 0b111:
                        pc+=(r[i.R.rs1] >= r[i.R.rs2])*imme;
                            //sprintf(step,"bran big or equal unsigned imm:%i,new pc is %i,r%i vs r%i",immeSB,pc,i.R.rs1,i.R.rs2);
                        
                
                    break;default:
                    invalid_instruction(i.whole,r,pc);
                    break;
                }

            break;case CUJ:
                r[i.R.rd] = pc+4;
                imme = i.UJ.imm1*2 + i.UJ.imm12*4096 + i.UJ.imm20*1048576 + i.UJ.imm11*2048;
                imme = (imme>= 1048576)*(0xFFE00000|imme)+(imme<1048576)*imme;
                pc = pc + imme - 4;
                //sprintf(step,"jum n link,saved %i to r%i,new pc is%i,imm:%i",r[i.R.rd],i.R.rd,pc,imme);

            break;case CJR:
                if (!i.R.func3){
                    r[i.R.rd] = pc+4;
                    pc= r[i.R.rs1] + sext12_to_32(i.I.imm) - 4 ;
                    //sprintf(step,"jum n lin reg,saved %i to r%i,new pc is%i,imm:%i",r[i.R.rd],i.R.rd,pc,sext12_to_32(i.I.imm));
                    break;
                }
                
            default:
                invalid_instruction(i.whole,r,pc);
            break;
        }
        //printf("%s\n",step);
        pc+=4;
    }
    //printf("pc out of bound: %i",pc);

}