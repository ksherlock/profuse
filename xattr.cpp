#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/xattr.h>


void dumpxattr(const char *file, const char *attr)
{
ssize_t asize;
char *buffer;

    asize = getxattr(file, attr, NULL, 0);
    if (asize < 0) return;
    
    printf("%s: %d\n", attr, asize);

    buffer = (char *)malloc(asize);
    if (!buffer) return;
    
    if ( (asize = getxattr(file, attr, buffer, asize)) > 0)
    {
        char *cp = buffer;
        char buffer1[16 * 3 + 1 + 1];
        char buffer2[16 + 1];
        ssize_t offset = 0;
        unsigned i, j;
        unsigned max;
        
        while(asize > 0)
        {        
            memset(buffer1, ' ', sizeof(buffer1));
            memset(buffer2, ' ', sizeof(buffer2));
            
            max = asize > 16 ? 16 : asize;
            
            
            for (i = 0, j = 0; i < max; i++)
            {
                unsigned x = cp[i];
                buffer1[j++] = "0123456789abcdef"[x >> 4];
                buffer1[j++] = "0123456789abcdef"[x & 0x0f];
                j++;
                if (i == 7) j++;
                
                buffer2[i] = isprint(x) ? x : '.';
                
            }
            
            buffer1[sizeof(buffer1)-1] = 0;
            buffer2[sizeof(buffer2)-1] = 0;
            

            printf("%06x:\t%s\t%s\n", offset, buffer1, buffer2);
            offset += 16;
            cp += 16;
            asize -= 16;
        }
    } 
    
    free(buffer);
}


void listx(const char *file)
{
ssize_t asize;
char *buffer;

    asize = listxattr(file, NULL, 0);
    
    if (asize < 0) return;


    buffer = (char *)malloc(asize);
    
    if (!buffer) return;
    
    if ((asize = listxattr(file, buffer, asize)) > 0)
    {    
        char *cp = buffer;
        ssize_t len;
        
        while (asize > 0)
        {
            dumpxattr(file, cp);
            len = strlen(cp) + 1;
            cp += len;
            asize -= len;
        }
    }
    
    
    free(buffer);
}

int main(int argc, char **argv)
{
struct stat st;
unsigned i;
ssize_t asize;


  for (i = 1; i < argc; ++i)
  {
    if (stat(argv[i], &st) != 0)
    {
      fprintf(stderr, "Unable to stat %s\n", argv[i]);
      continue;
    }

    printf("%s\n", argv[i]);
    printf("\tChange Time      : %s\n", asctime(localtime(&st.st_ctime)));
    printf("\tModification Time: %s\n", asctime(localtime(&st.st_mtime)));
    printf("\tAccess Time      : %s\n", asctime(localtime(&st.st_atime)));

    listx(argv[i]);
    printf("\n");
  }


 return 0;
}
