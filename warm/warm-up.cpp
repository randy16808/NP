#include<stdio.h>
#include<string.h>
#include<stdlib.h>

void reverse(char * str)
{
    int i=0, j=strlen(str)-1;
    char c;
    for( ; i<j ; ++i, --j)
    {   c=str[i]; str[i]=str[j]; str[j]=c; }
}

void tolk(char* str, char * s)
{
    char * pch = strtok(str, s);
    while(pch != NULL)
    {
        printf("%s ", pch);
        pch = strtok(NULL, s);
    }
}    

int main(int argc, char** argv)
{
    FILE * pFile;
    char mystring[100],str[100];
    if(argc > 2)
    {
        pFile = fopen(argv[1], "r");
        while(fgets(mystring, 100, pFile) != NULL)
        {
             if(mystring[0]=='r')
             {
                 mystring[strlen(mystring)-1]='\0';
                 strcpy(str, mystring+8);
                 reverse(str);
                 puts(str);
             }
             else if(mystring[0]=='s')
             {
                mystring[strlen(mystring)-1]='\0';
                strcpy(str, mystring+6);
                tolk(str, argv[2]);
                printf("\n");
            }
            else if(mystring[0]=='e') exit(0);
        }
        fclose(pFile);
    }
    while(scanf("%s", mystring))
    {
        if(mystring[0]=='e') exit(0);
        else if(mystring[0]=='r')
        {   

            fgets(mystring, 100, stdin);
            mystring[strlen(mystring)-1]='\0';
           // strcpy(str, mystring+8);
            reverse(mystring); puts(mystring);
        }
        else
        {
            scanf("%s", str);
            tolk(str, argv[2]);
            printf("\n");
        }
    }
    return 0;
}    

