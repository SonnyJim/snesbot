/*
 * pidfile.c:
 *
 *
 * Copyright (c) 2016 Ewan Meadows
 ***********************************************************************
 *
 *    This is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with this.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#define PIDFILE "/tmp/snesbot.pid"
#define _POSIX_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

int remove_pid (void)
{
  if (remove (PIDFILE) != 0)
  {
    fprintf (stderr, "Error removing PID file %s: %s\n", PIDFILE, strerror(errno));
    return 1;
  }
  else
    return 0;
}

int check_pid (void)
{
    FILE *fd;
    int pid = getpid ();
    int oldpid;
    
    //Check to see if pidfile already exists
    fd = fopen (PIDFILE, "r");
    if (fd != NULL)
    {  
        //Read pid into oldpid
        fscanf (fd, "%d", &oldpid);
        
        //Check to see if the old process is still running
        if (pid != oldpid && kill (oldpid, 0) == 0)
        {
            fprintf (stderr, "Already running as PID %d\n", oldpid);
            fclose(fd);
            return 1;
        }
        else
        {
            fclose(fd);
            remove (PIDFILE);
        }
    }

    //Lets make a new pidfile!
    fd = fopen (PIDFILE, "w");
    if (fd == NULL)
    {
        fprintf(stderr, "Error: Can't open or create %s: %s\n", PIDFILE, strerror(errno));
        return 1;
    }
   
    //Write PID to file
    fprintf (fd, "%d\n", pid);
    fclose (fd);
    return 0;
}
