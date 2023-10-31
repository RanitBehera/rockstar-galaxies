/*
 * MPGADGET I/O for Rockstar
 * Ranit Behera (ranit.behera@iucaa.in)
 */


#include <stdio.h>          // To print basic run status
#include <stdlib.h>         // Required for heap malloc
#include <string.h>         // Required for strlen()

#include "io_mpgadget.h"    // Following structure

// Required for Particle definition
// All particle proprties like pos, mass, id, type are defined here
#include "../particle.h"

// All config parameters from config file
// like Ol,Om,h0,SCALE_NOW, OUTBASE, INBASE etc
// are declared here via ../config.template.h via ../config_vars.h
// Set default values there for MPGADGET fields
#include "../config_vars.h"  
#include "../check_syscalls.h"  // for check_fopen()
#include "../universal_constants.h"  // for CRITICAL_DENSITY etc


#define MPGADGET_NTYPES 6
#define MPGHPT MPGADGET_HALO_PARTICLE_TYPE
#define FILE_NAME_MAX_LENGTH 6

// joins to strings and returns the pointer to heap
char* str_join_on_heap(char *str1,char *str2){
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    const int total_len = len1 + len2 + 1; //+1 for terminating null char
    char* joined_string = (char*)malloc(total_len);
    int i,j;
    for (i=0;i<len1;i++){joined_string[i]=str1[i];}     //i=len1 when loop breaks
    for (j=0;j<len2;j++){joined_string[i+j]=str2[j];}   //j=len2 when loop breaks
    joined_string[i+j]='\0';    // terminating null char
    return joined_string;       // rember to free heap memory
}

void mpgadget_read_snap_header(char *filename,float *massTable, int64_t *npart, int64_t *npart_init)
{
    char *subdir="/Header/attr-v2";
    char *fullpath = str_join_on_heap(filename,subdir);

    FILE* input;
    char header_line[200];  // Hard coded

    input = check_fopen(fullpath,"r");
    while (fgets(header_line, 256, input)){  // fgets
        //SCALAR
        if(!strncmp(header_line,"BoxSize",7)){
            strtok(header_line,"[");
            BOX_SIZE=atof(strtok(NULL,"]"));
            BOX_SIZE*=MPGADGET_LENGTH_CONVERSION;
            continue;
        }
        if(!strncmp(header_line,"HubbleParam",11)){
            strtok(header_line,"[");
            h0=atof(strtok(NULL,"]"));
            continue;
        }
        if(!strncmp(header_line,"Omega0",6)){
            strtok(header_line,"[");
            Om=atof(strtok(NULL,"]"));
            continue;
        }
        if(!strncmp(header_line,"OmegaLambda",11)){
            strtok(header_line,"[");
            Ol=atof(strtok(NULL,"]"));
            continue;
        }
        if(!strncmp(header_line,"Time",4) && strncmp(header_line,"TimeIC",6)){
            strtok(header_line,"[");
            SCALE_NOW=atof(strtok(NULL,"]"));
            continue;
        }
        // ARRAY
        if(!strncmp(header_line,"MassTable",9)){
            strtok(header_line,"[");
            for(int i=0;i<MPGADGET_NTYPES;i++){
                massTable[i]=atof(strtok(NULL," "));
            }
            continue;
        }
        if(!strncmp(header_line,"TotNumPart",10) && strncmp(header_line,"TotNumPartInit",14)){
            strtok(header_line,"[");
            for(int i=0;i<MPGADGET_NTYPES;i++){
                npart[i]=atoi(strtok(NULL," "));
            }
            continue;
        }
        if(!strncmp(header_line,"TotNumPartInit",14)){
            strtok(header_line,"[");
            for(int i=0;i<MPGADGET_NTYPES;i++){
                npart_init[i]=atoi(strtok(NULL," "));
            }
            continue;
        }
    }

    // printf("\nBS=%lf, h=%lf, Ol=%lf, Om=%lf, time=%lf\n", BOX_SIZE,h0,Ol,Om,SCALE_NOW);
    // printf("\nm0=%lf, m1=%lf, m2=%lf, m3=%lf, m4=%lf, m5=%lf\n", massTable[0], massTable[1], massTable[2], massTable[3], massTable[4], massTable[5]);
    // printf("\nm0=%d, m1=%d, m2=%d, m3=%d, m4=%d, m5=%d\n", npart[0], npart[1], npart[2], npart[3], npart[4], npart[5]);
    // printf("\nm0=%d, m1=%d, m2=%d, m3=%d, m4=%d, m5=%d\n", npart_init[0], npart_init[1], npart_init[2], npart_init[3], npart_init[4], npart_init[5]);

    fclose(input);
    free(fullpath);
}

void mpgadget_read_field_header(char* fielddir, char* DTYPE, int* NMEMB, int* NFILE, int64_t** LPF, char*** DFN){
    char* file=str_join_on_heap(fielddir,"header");
    FILE* input;
    char header_line[256];
    char* token;

    int i=0;
    input = check_fopen(file,"r");
    while(fgets(header_line, 256, input)){
        token=strtok(header_line,":");
        if(!strcmp(token,"DTYPE")){strcpy(DTYPE,strtok(NULL,"\n"));continue;}
        if(!strcmp(token,"NMEMB")){*NMEMB=atoi(strtok(NULL,"\n"));continue;}
        if(!strcmp(token,"NFILE")){
            *NFILE=atoi(strtok(NULL,"\n"));
            *DFN=(char**)malloc((*NFILE)*sizeof(char*));
            for (int j = 0; j < *NFILE; j++) {*((*DFN)+j) = (char*) malloc((6+1) * sizeof(char));}
            *LPF=(int*)malloc(*NFILE);
            continue;
        }
        strcpy(*((*DFN)+i),token);
        *((*LPF)+i)=atoi(strtok(NULL,":"));i++;
    }
    fclose(input);
    free(file);
}


void load_particles_mpgadget(char *filename, struct particle **p, int64_t *num_p)
{

    int64_t npart[MPGADGET_NTYPES];
    int64_t npart_init[MPGADGET_NTYPES];
    float massTable[MPGADGET_NTYPES];

    mpgadget_read_snap_header(filename,massTable,npart,npart_init);
    PARTICLE_MASS   = massTable[MPGHPT] * MPGADGET_MASS_CONVERSION;
    AVG_PARTICLE_SPACING = cbrt(PARTICLE_MASS / (Om*CRITICAL_DENSITY));

    if(RESCALE_PARTICLE_MASS){
        PARTICLE_MASS = Om*CRITICAL_DENSITY * pow(BOX_SIZE, 3) / TOTAL_PARTICLES;
    }

    TOTAL_PARTICLES = 0;
    for (int64_t i=0; i<MPGADGET_NTYPES; i++) {
        TOTAL_PARTICLES +=(int64_t)npart[i];
    }

    printf("MPGADGET: snapname:       %s\n", filename);
    printf("MPGADGET: box size:       %g Mpc/h\n", BOX_SIZE);
    printf("MPGADGET: h0:             %g\n", h0);
    printf("MPGADGET: Om:             %g\n", Om);
    printf("MPGADGET: Ol:             %g\n", Ol);
    printf("MPGADGET: scale factor:   %g\n", SCALE_NOW);
    printf("MPGADGET: Total Part:     %" PRIu64 "\n", TOTAL_PARTICLES);
    printf("MPGADGET: DM Part Mass:   %g Msun/h\n", PARTICLE_MASS);
    printf("MPGADGET: avgPartSpacing: %g Mpc/h\n\n", AVG_PARTICLE_SPACING);
  
    // Check to_read variable method in arepo for if that is more effiecient
    check_realloc_s(*p, TOTAL_PARTICLES, sizeof(struct particle));
    memset(*p, 0, sizeof(struct particle)*TOTAL_PARTICLES);

    *num_p = TOTAL_PARTICLES;



    char DTYPE[4];
    int NMEMB, NFILE;
    int64_t* LPF;   // Length Per File
    char** DFN; // Date File Name

    char partdir[MPGADGET_NTYPES][4]={"/0/","/1/","/2/","/3/","/4/","/5/"};
    char fielddir[4][16]={"ID/","Mass/","Position/","Velocity/"};

    char *subdir,*fulldir,*file;
    FILE* ptr;
    int64_t porigin=0,forigin=0;
    for (int ptype=0;ptype<MPGADGET_NTYPES;ptype++){
        if (!npart[ptype]){continue;}   // Dont proceed of zero particle of given type
        int32_t type = RTYPE_DM;
        if (ptype==0) type = RTYPE_GAS;
        else if (ptype==4) type = RTYPE_STAR;
        else if (ptype==5) type = RTYPE_BH;

        for(int64_t pi=0;pi<npart[ptype];pi++){
            ((*p)+porigin+pi)->type=type;
        }
        
        for (int ftype=0;ftype<4;ftype++){
            subdir=str_join_on_heap(partdir[ptype],fielddir[ftype]);
            fulldir=str_join_on_heap(filename,subdir);
            // printf("%s\n",fulldir);

            mpgadget_read_field_header(fulldir,&DTYPE,&NMEMB,&NFILE,&LPF,&DFN);
            // printf("DTYPE:%s\nNMEMB: %d\nNFILE: %d\n",DTYPE,NMEMB,NFILE);        // Uncomment these two lines
            // for(int i=0;i<NFILE;i++){printf("%s: %d\n",*(DFN+i),*(LPF+i));}      // To print field header
            
            forigin=0;
            for(int nfile=0;nfile<NFILE;nfile++){
                file=str_join_on_heap(fulldir,*(DFN+nfile));
                // printf("Reading : %s\n",file);
                
                ptr=check_fopen(file,"rb");

                if(ftype==0){
                    int64_t* buffer=(int64_t *)malloc(sizeof(int64_t)*NMEMB*(*(LPF+nfile)));
                    fread(buffer,sizeof(int64_t)*NMEMB,*(LPF+nfile),ptr);
                    for(int64_t bi=0;bi<*(LPF+nfile);bi++){
                        ((*p)+porigin+forigin+bi)->id=*(buffer+bi);
                    }
                    free(buffer);
                }
                else{
                    float* buffer=(float *)malloc(sizeof(float)*NMEMB*(*(LPF+nfile)));
                    fread(buffer,sizeof(float)*NMEMB,*(LPF+nfile),ptr);
                    for(int64_t bi=0;bi<*(LPF+nfile);bi++){
                        if(ftype==1){
                            ((*p)+porigin+forigin+bi)->mass=*(buffer+bi);continue;
                        }
                        if (ftype==2){
                            ((*p)+porigin+forigin+bi)->pos[0]=*(buffer+bi)*MPGADGET_LENGTH_CONVERSION;bi++;
                            ((*p)+porigin+forigin+bi)->pos[1]=*(buffer+bi)*MPGADGET_LENGTH_CONVERSION;bi++;
                            ((*p)+porigin+forigin+bi)->pos[2]=*(buffer+bi)*MPGADGET_LENGTH_CONVERSION;continue;
                        }
                        if (ftype==3){
                            ((*p)+porigin+forigin+bi)->pos[3]=*(buffer+bi);bi++;
                            ((*p)+porigin+forigin+bi)->pos[4]=*(buffer+bi);bi++;
                            ((*p)+porigin+forigin+bi)->pos[5]=*(buffer+bi);continue;
                        }
                    }
                    free(buffer);
                }

                fclose(ptr);
                free(file);
                forigin+=*(LPF+nfile);
            }
            
            free(LPF);
            for (int i = 0; i < NFILE; i++) {free(*(DFN+i));}
            free(DFN);
            free(fulldir);
            free(subdir);
        }
        porigin+=npart[ptype];
    }


    // Debug



    printf("exited\n");
    // exit(1);
}