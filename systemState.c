
#define _POSIX_C_SOURCE 200809L

#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<signal.h>
#include<time.h>
#include<errno.h>

#include<sys/wait.h>

#include"config.h"
#include"systemState.h"
#include"jsonParser.h"
#include"strUtil.h"

ssize_t execute(cchar* command, cchar* arg1, cchar* arg2, char* buffer, size_t bufSize)
{
  int fd[2];
  pipe(fd);
  int childpid = fork();
  if(childpid == -1) //Fail
  {
     fprintf(stderr, "FORK failed");
     return 0;
  } 
  else if(childpid == 0) //I am child
  {
    close(1); //stdout
    dup2(fd[1], 1);
    close(fd[0]);
    execlp(shell_file_loc, shell_file_loc, "-c", command, arg1, arg2, (char*)NULL);
    //child has been replaced by shell command
  }

  wait(NULL);
  errno = EINTR;
  ssize_t readBytes = 0;
  ssize_t n = 0;
  while(errno == EAGAIN || errno == EINTR)
  {
    errno = 0;
    n = read(fd[0], buffer, bufSize-1);
    if(n == 0)
      break;
    readBytes += n;
  }
  buffer[readBytes] = '\0';
  return readBytes;
}

/******************************************************************************/
/******************************************************************************/

/**
 * Gets the ip address ot the given interface.
 * Returns the pointer to the result.
 * Currently a very bad algorithm. 
 * Please suggest a better one.
 */
void getIpAddress(cchar* interface, char* result)
{
  char buf[1000] = { 0 };
  char command[40] = { 0 };
  sprintf(command, ifconfig_file_loc, interface);
  execute(command, NULL, NULL, buf, 1000);
  
  char* c = buf;
  while(*c != '\n') c++;
  skipWhitespaces(&c);
  int ip1, ip2, ip3, ip4;
  int matched = sscanf(c, "inet addr:%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
  if(matched != 4)
    fprintf(stderr, "Ip address not matched (%d)\n%s\n", matched, c);

  sprintf(result, "%d.%d.%d.%d",ip1, ip2, ip3, ip4);
}

/**
 * Queries iwconfig for the essid and the quality of the given interface
 */
void getWirelessState(cchar* interface, int* quality, char* result, size_t resLen)
{
  char buf[1000] = { 0 };
  char command[40] = { 0 };
  sprintf(command, iwconfig_file_loc, interface);
  execute(command, NULL, NULL, buf, 1000);
  
  *quality = -1;
  char* c = buf;
  
  while(1)
  {
    skipWhitespaces(&c);
    if(*c == '\0')
      break;
    else if(strncmp(c, "ESSID:", 6) == 0)
    {
      c+=7;
      char* e = c;
      while(*e != '"')
        e++;
        
      size_t length = (size_t)(e-c) < resLen-1 ? (size_t)(e-c) : resLen-1;
      strncpy(result, c, length);
      result[length] = '\0';
    }
    else if(strncmp(c, "Link Quality=", 13) == 0)
    {
      c+=13;
      int q1 = (int)strtol(c, &c, 0);
      c++;
      int q2 = (int)strtol(c, &c, 0);
      *quality = 100*q1 / q2;
      return; //assuming that quality comes after essid
    }
    skipNonWhitespace(&c);
  }
  
  return;
}

/******************************************************************************/
/******************************************************************************/

void getSpaceUsage(cchar* mount_point, char* size, char* used)
{
  char buf[1000] = { 0 };
  char command[40] = { 0 };
  sprintf(command, getSpace, mount_point);
  execute(command, NULL, NULL, buf, 1000);
  
  char* c = buf;
  while(*c != '\n') c++;
  c++;
  skipNonWhitespace(&c);
  skipWhitespaces(&c);
  char* beg = c;
  skipNonWhitespace(&c);
  memcpy(size, beg, c-beg);
  skipWhitespaces(&c);
  beg = c;
  skipNonWhitespace(&c);
  memcpy(used, beg, c-beg);
  return;
}

/******************************************************************************/
/******************************************************************************/

void init_systemState(SystemState* sys)
{
  pthread_mutex_init(&sys->mutex, NULL);
  pthread_mutex_lock(&sys->mutex);
  
  sys->mute = 0;
  sys->volume = 0;
  
  memset(sys->time, 0, 100);
  sys->cpu_temp = 0;
  sys->cpu_load = 0.0f;
  sys->bat_status = NO_BATTERY;
  sys->bat_discharge = 0;
  sys->bat_capacity = 0;
  sys->bat_level = 0;
  
  sys->net_eth_up = 0;
  memset(sys->net_eth_ip, 0, 40);
  sys->net_wlan_up = 0;
  memset(sys->net_wlan_essid, 0, 100);
  sys->net_wlan_quality = 0;
  memset(sys->net_wlan_ip, 0, 40);
  
  memset(space_home_size, 0, 10);
  memset(space_home_used, 0, 10);
  memset(space_root_size, 0, 10);
  memset(space_root_used, 0, 10);
  
  pthread_mutex_unlock(&sys->mutex);
}

/**
 * Queries the system resources
 */
void update_systemState(SystemState* sys)
{
  pthread_mutex_lock(&sys->mutex);

  char line[1000] = { 0 };
  
  //Date
  ssize_t rb = execute(getDate, NULL, NULL, sys->time, sizeof(sys->time));
  sys->time[rb-1] = '\0';
  
  //Temp
  FILE* tfile = fopen(temp_file_loc, "r");
  fgets(line, 10, tfile);
  fclose(tfile);
  sys->cpu_temp = (int)strtol(line, NULL, 0) / 1000;
  
  FILE* lfile = fopen(load_file_loc, "r");
  fgets(line, 10, lfile);
  fclose(lfile);
  sys->cpu_load = strtof(line, NULL);
  
  //battery
  FILE* bfile = fopen(bat_file_loc, "r");
  if(bfile == NULL)
    sys->bat_status = NO_BATTERY; //Probably no battery
  else
  {
    while(fgets(line, 200, bfile) != NULL)
    {
      if(strncmp(line+13, "STATUS", 6) == 0)
      {
        if(strncmp(line+20, "Full", 4) == 0)
          sys->bat_status = BATTERY_FULL;
        else if(strncmp(line+20, "Charging", 8) == 0)
          sys->bat_status = BATTERY_CHARGING;
        else if(strncmp(line+20, "Discharging", 11) == 0)
          sys->bat_status = BATTERY_DISCHARGING;
        else
          sys->bat_status = BATTERY_FULL;
      }
      else if(strncmp(line+13, "POWER_NOW", 9) == 0)
        sys->bat_discharge = strtol(line+23, NULL, 0);
      else if(strncmp(line+13, "ENERGY_FULL_DESIGN", 18) == 0)
        sys->bat_capacity = strtol(line+32, NULL, 0);
      else if(strncmp(line+13, "ENERGY_NOW", 10) == 0)
        sys->bat_level = strtol(line+24, NULL, 0);
    }
    fclose(bfile);
  }
  
  sprintf(line, ifstat_file_loc, ethernet);
  FILE* ethfile = fopen(line, "r");
  fgets(line, 10, ethfile);
  fclose(ethfile);
  if((sys->net_eth_up = strncmp(line, "up", 2) == 0))
    getIpAddress(ethernet, sys->net_eth_ip);

  sprintf(line, ifstat_file_loc, wireless);
  FILE* wlanfile = fopen(line, "r");
  fgets(line, 10, wlanfile);
  fclose(wlanfile);
  if((sys->net_wlan_up = strncmp(line, "up", 2) == 0))
  {
    getIpAddress(wireless, sys->net_wlan_ip);
    getWirelessState(wireless, &sys->net_wlan_quality, sys->net_wlan_essid, sizeof(sys->net_wlan_essid));
  }
  
  getSpaceUsage(root_dir, sys->space_root_size, sys->space_root_used);
  getSpaceUsage(home_dir, sys->space_home_size, sys->space_home_used);

  pthread_mutex_unlock(&sys->mutex);
}

void update_constSystemState(SystemState* sys)
{
  (void)sys;
  getSpaceUsage(root_dir, sys->space_root_size, sys->space_root_used);
  getSpaceUsage(home_dir, sys->space_home_size, sys->space_home_used);
}

void free_systemState(SystemState* sys)
{
  pthread_mutex_destroy(&sys->mutex);
}


