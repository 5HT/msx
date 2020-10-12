/*********************************************************************
*  MIMSXPC: this program allows you to retrieve MSX files stored in  *
*           single-sided disks to PC.                                *
**********************************************************************
*                       ,         ,                                  *
* Author: Guillermo Gonzalez Talavan, at gyermo@gugu.usal.es.        *
*********************************************************************/

#include <stdio.h>
#include <dir.h>
#include <string.h>
#include <dos.h>

int getnom (char entrada[],char nom[]);
int estaarch (char nom[],char dir[]);
void ponfich (char nf[]);
int cogefat (int fat[]);
int copiar (char nombre[],int topcluster,int sector,unsigned long longi);

main(int argc,char *argv[])
{
        unsigned long longi,*l;
        int err,topcluster,*p,sector,i;
        char nom[12],dir[33];

        if (argc!=2) {printf("The correct format is: MIMSXPC name_of_file.\n");
                      return 1;}
        err=getnom(argv[1],nom);
        if (err) return 1;
        err=estaarch(nom,dir);
        if (err) return 1;
        p=(int *)(dir+26);
        topcluster=*p;
        sector=12+(topcluster-2)*2;
        printf("Boot sector is:%i.\n",sector);
        l=(long *)(dir+28);
        longi=*l;
        printf("Program length is %li bytes.\n",longi);
        err=copiar(argv[1],topcluster,sector,longi);
        if (err) return 1;
        return 0;
}

int getnom (char entrada[],char nom[])
        {int err;
        char unidad[MAXDRIVE],dir[MAXDIR],nombrear[MAXFILE],ext[MAXEXT];

        err=fnsplit(entrada,unidad,dir,nombrear,ext);
        if (err & DRIVE) {printf("Only unit A: is allowed.\n");
                return -1;}
        if (err & DIRECTORY) {printf("MSX-DOS doesn't permit subdirectories.\n");
                return -1;}
        if (!(err & (FILENAME | EXTENSION))) {printf("Wrong file format.\n");
                return -1;}
        if (err & WILDCARDS) {printf("Wild cards not allowed here.\n");
                return -1;}
        strcpy(nom,nombrear);
        if (strlen(nom)<8) strncat(nom,"        ",8-strlen(nom));
        if (strlen(ext)) {strncat(nom,(ext+1),3);
                if (strlen(nom)<11) strncat(nom,"   ",11-strlen(nom));}
        else strncat(nom,"   ",3);
        strupr(nom);
        return 0;}

int estaarch (char nom[],char dir[33])
        {int sector,i,j,err,salir;
        unsigned char buf[17][32],nom2[12];

        salir=0;
        for (sector=5;sector<12;sector++)
                {err=absread(0,1,sector,buf);
                if (err) return err;
                for (i=0;i<16;i++)
                        {strcpy(nom2,"");strncat(nom2,buf[i],11);
                        if (*nom2 == 0) salir=-1;
                        if (*nom2 == 0) break;
                        if (*nom2 != 229) 
                                {ponfich(nom2);
                                if (!strcmp(nom,nom2))
                                        {printf("\n");
                                        for (j=0;j<32;j++) dir[j]=buf[i][j];
                                        dir[32]=0;
                                        return 0;}
                                }
                        }
                if (salir) break;}
        printf("\n\nCan't find requested file. Try again.\n");
        return -1;}

void ponfich (char nf[])
        {char nombre[9];

        strcpy(nombre,"");strncat(nombre,nf,8);
        printf("  ");printf(nombre);printf(".");printf((nf+8));printf("  ");
        return;}

int cogefat(int fat[])
        {unsigned char codfat[1024],*pc;
        int err,*p,i;

        err=absread(0,2,1,codfat);
        if (err) return -1;
        p=fat;pc=codfat;
        for (i=0;i<682;i+=2)
                {*p=*pc+(*(pc+1) & 15)*256;p++;
                *p=*(pc+1)/16+*(pc+2)*16;p++;
                pc+=3;}
        return 0;}

int copiar(char nombre[],int topcluster,int sector,unsigned long longi)
        {FILE *objeto;
        int fat[684],err;
        unsigned long transferido;
        char buf[1024];

        if (longi>1024)
                {err=cogefat(fat);
                if (err)
                        {printf("Can't load F.A.T.!\n");
                        return -1;}}
        if ((objeto=fopen(nombre,"wb"))==NULL)
                {printf("Object file can4t be opened.\n");
                return -1;}
        for(transferido=0;transferido<longi;)
                {err=absread(0,2,sector,buf);
                if (err)
                        {printf("Disk sector read error.\n");
                        return -1;}
                if (longi-transferido<=1024) 
                        {fwrite(buf,(longi-transferido),1,objeto);
                        transferido=longi;}
                else 
                        {fwrite(buf,1024,1,objeto);
                        transferido+=1024;
                        topcluster=fat[topcluster];
                        if (topcluster==4095)
                                {printf("Found corrupted F.A.T.\n");
                                return -1;}
                        sector=2*(topcluster-2)+12;}
                }
        if (fclose(objeto)==EOF) {printf("Not enough disk space.\n");return -1;}
        return 0;}

