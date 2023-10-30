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

float mpgadget_read_snap_header(char *filename,float *massTable, int64_t *npart, int64_t *npart_init)
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
    return 0;
}



void mpgadget_read_data(char* filename,int part_type, char* field){
    
    char *particle_dir;
    if (part_type==0){particle_dir="/0/";}
    if (part_type==1){particle_dir="/1/";}  
    if (part_type==2){particle_dir="/2/";}  
    if (part_type==3){particle_dir="/3/";}  
    if (part_type==4){particle_dir="/4/";}
    if (part_type==5){particle_dir="/5/";}
    char* subdir=str_join_on_heap(particle_dir,field);
    char *fulldir = str_join_on_heap(filename,subdir);   free(subdir);
    char *file = str_join_on_heap(fulldir,"header");

    FILE* input;

    char header_line[256];
    char* DTYPE;
    int NMEMB, NFILE;
    char** datafile;
    int* LPF;   //Length per file for number of particles in each file
    int fn=0;   //file number for each line of file info in header

    // Read Header
    input = check_fopen(file,"r");
    while (fgets(header_line, 256, input)){
        if(!strncmp(header_line,"DTYPE",5)){
            strtok(header_line,"<");
            DTYPE=strtok(NULL,"\n");
            continue;
        }
        if(!strncmp(header_line,"NMEMB",5)){
            strtok(header_line,":");
            NMEMB=atoi(strtok(NULL,"\n"));
            continue;
        }
        if(!strncmp(header_line,"NFILE",5)){
            strtok(header_line,":");
            NFILE=atoi(strtok(NULL,"\n"));

            datafile=(char**)malloc(NFILE*sizeof(char*));
            for (int i = 0; i < NFILE; i++) {
                *(datafile+i) = (char*) malloc((6+1) * sizeof(char));
            }
            LPF=(int*)malloc(NFILE);
            continue;
        }
        strcpy(*(datafile+fn),strtok(header_line,":"));
        *(LPF+fn)=atoi(strtok(NULL,":"));fn++;
    }
    fclose(input);free(file);

    //Read data
    // *(datafile+i) contains filenames like 000001,00000A as char*

    for(int i=0;i<NFILE;i++){
        file=str_join_on_heap(fulldir,*(datafile+i));printf("%s\n",file);
        
    }
    free(file);

    for (int i = 0; i < NFILE; i++) {free(*(datafile+i));}
    free(datafile);
    free(LPF);
    free(fulldir);
    exit(1);
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

  for (int64_t i=0; i<MPGADGET_NTYPES; i++){
    // // read IDs, pos, vel    
    // char buffer[100];
    // int32_t type = RTYPE_DM;
    // if (i==0) type = RTYPE_GAS;
    // else if (i==4) type = RTYPE_STAR;
    // // else if (i==5) type = RTYPE_BH;
  
    // if (!npart[i]) continue;
    // snprintf(buffer, 100, "PartType%"PRId64, i);

    // continue if no particle type
    if (!npart[i]){continue;}

    if (i==0){//Gas
        // for(;(*num_p)<npart[i];(*num_p)++){
        //     (*(p+(*num_p)))->id;
        // }

       mpgadget_read_data(filename,0,"ID/");
    }



    
  }




  exit(1);
}