#include <stdio.h> 
#include <string.h> 

//Checks /proc/cpuinfo for BCM2708, which should make it a Pi
//TODO check revision
int is_a_pi (void)
{
   FILE* fp; 
   char buffer[4096]; 
   size_t bytes_read; 
   char* match; 
   
   fp = fopen ("/proc/cpuinfo", "r"); 
   bytes_read = fread (buffer, 1, sizeof (buffer), fp); 
   fclose (fp); 

   if (bytes_read == 0 || bytes_read == sizeof (buffer)) 
   {
       printf ("Read of /proc/cpuinfo failed\n");
       return 0; 
   }
   
   buffer[bytes_read] = '\0';
   
   match = strstr (buffer, "BCM2708"); 
   if (match == NULL) 
       return 0; 
   return 1;
}
