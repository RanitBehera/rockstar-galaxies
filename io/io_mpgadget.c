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

void mpgadget_read_field_header(char* filename, char* field_dir, char* DTYPE, int* NMEMB, int* NFILE, int** LPF, char*** DFN){
    char* dir=str_join_on_heap(filename,field_dir);
    char* file=str_join_on_heap(dir,"header");

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
    free(dir);
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

  int64_t to_read = 0;
  TOTAL_PARTICLES = 0;
  for (int64_t i=0; i<MPGADGET_NTYPES; i++) {
    to_read += npart[i];
    TOTAL_PARTICLES +=(int64_t)npart[i];
  }

  printf("MPGADGET: snapname:       %s\n", filename);
  printf("MPGADGET: box size:       %g Mpc/h\n", BOX_SIZE);
  printf("MPGADGET: h0:             %g\n", h0);
  printf("MPGADGET: scale factor:   %g\n", SCALE_NOW);
  printf("MPGADGET: Total Part:     %" PRIu64 "\n", TOTAL_PARTICLES);
  printf("MPGADGET: ThisFile Part:  %" PRIu64 "\n", to_read);
  printf("MPGADGET: DM Part Mass:   %g Msun/h\n", PARTICLE_MASS);
  printf("MPGADGET: avgPartSpacing: %g Mpc/h\n\n", AVG_PARTICLE_SPACING);
  
  check_realloc_s(*p, ((*num_p)+to_read), sizeof(struct particle));
  memset((*p)+(*num_p), 0, sizeof(struct particle)*to_read);


//---------------------------------------------------------------------------
    char DTYPE[4];
    int NMEMB, NFILE;
    int* LPF;   // Length Per File
    char** DFN; // Date File Name

    mpgadget_read_field_header(filename,"/0/ID/",&DTYPE,&NMEMB,&NFILE,&LPF,&DFN);

    // printf("DTYPE:%s\nNMEMB: %d\nNFILE: %d\n",DTYPE,NMEMB,NFILE);        // Unceomment these two lines
    // for(int i=0;i<NFILE;i++){printf("%s: %d\n",*(DFN+i),*(LPF+i));}      // To print field header



    // open individual file and assign


//   for (int64_t i=0; i<MPGADGET_NTYPES; i++){
//     // // read IDs, pos, vel    
//     // char buffer[100];
//     // int32_t type = RTYPE_DM;
//     // if (i==0) type = RTYPE_GAS;
//     // else if (i==4) type = RTYPE_STAR;
//     // // else if (i==5) type = RTYPE_BH;
//     // snprintf(buffer, 100, "PartType%"PRId64, i);

//     // continue if no particle type
//     if (!npart[i]){continue;}


//     if (i==0){//Gas
//         char* databuffer;
//        datampgadget_read_data(filename,0,"ID/",&databuffer);

//         // for(;(*num_p)<npart[i];(*num_p)++){
//         //     (*(p+(*num_p)))->id;
//         // }


//     }
    
//   }


    free(LPF);
    for (int i = 0; i < NFILE; i++) {free(*(DFN+i));}
    free(DFN);

    exit(1);
}