#include <stdio.h>
#include <string.h>

int main(void)
{
  char output[100];
  printf("now call popen:\n");
  FILE *p = popen("sh ./runpy.sh", "r");
  if(p != NULL)
    {
      printf("print the output:\n");
      while(fgets(output, sizeof(output), p) != NULL)
      {
        printf("%s",output);
        if(NULL != strstr(output,"Python done."))
          printf("OK.\n");
        else
          printf("Error.\n");
      }
      printf("print the output end.\n");
    }
  pclose(p);


  printf("now call popen:\n");
  p = popen("sh ./runpy.sh 2>&1", "r");
  if(p != NULL)
    {
      printf("print the output:\n");
      while(fgets(output, sizeof(output), p) != NULL)
      {
        printf("%s",output);
        if(NULL != strstr(output,"Python done."))
          printf("OK.\n");
        else
          printf("Error.\n");
      }
      printf("print the output end.\n");
    }
    pclose(p);



  return 0;
}
