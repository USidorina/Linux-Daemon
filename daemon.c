#include <syslog.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

//sign = 0 (<) and sign = 1 (>) 
int moveFiles(const char sourceDir[], const char destinationDir[], bool sign, int timeThreshold) 
{
  DIR * dir1;
  DIR * dir2;

  struct dirent * dirent1;
  struct dirent * dirent2;

  time_t curTime = time(0);
  time_t lifeTime;

  int result = 0;
  struct stat info;

  char * path1 = NULL;
  char * path2 = NULL;

  if ((dir1 = opendir(sourceDir)) != NULL && (dir2 = opendir(destinationDir)) != NULL) {
    syslog(LOG_INFO, "Directory %s was open", sourceDir);
    while ((dirent1 = readdir(dir1)) !=  NULL) {
      if (strcmp(dirent1->d_name, ".") != 0 && strcmp(dirent1->d_name, "..") != 0) {

        path1 = (char*)malloc(strlen(sourceDir) + strlen(dirent1->d_name) + 2);
        path2 = (char*)malloc(strlen(destinationDir) + strlen(dirent1->d_name) + 2);

        if (path1 == NULL || path2 == NULL) {
          syslog(LOG_ERR, "Malloc returns NULL, errno - %d", errno);
          return 0;
        }

        path1 = strcpy(path1, sourceDir);
        path2 = strcpy(path2, destinationDir);

        path1 = strcat(path1, "/");
        path2 = strcat(path2, "/");

        path1 = strcat(path1, dirent1->d_name);
        path2 = strcat(path2, dirent1->d_name);

        if (stat(path1, &info) == -1) {
          syslog(LOG_ERR, "Stat returns -1 for %s, errno = %d", path1, errno);
          free(path1);
          free(path2);
          return 0;
        }

        lifeTime = (curTime - info.st_ctime) / 60;
        syslog(LOG_INFO, "Lifetime of file %s = %d minutes", path1, lifeTime);


        switch (sign) {
          case 1:
            if (lifeTime > timeThreshold) {
              result = rename(path1, path2);
              if (result == 0) {
                syslog(LOG_INFO, "Moving of file %s was successful", path1);
              } else {
                syslog(LOG_INFO, "Moving of file %s was failed", path1);
              }
            }
            break;

          case 0:
            if (lifeTime < timeThreshold) {
              result = rename(path1, path2);
              if (result == 0) {
                syslog(LOG_INFO, "Moving of file %s was successful", path1);
              } else {
                syslog(LOG_INFO, "Moving of file %s was failed", path1);
              }
            }
            break;
        }

        free(path1);
        free(path2);

        path1 = NULL;
        path2 = NULL;
      }
    }
    closedir(dir1);
    closedir(dir2);
  }

  return 1;
}


void sig_handler(int signo)
{
  if(signo == SIGTERM)
  {
    syslog(LOG_INFO, "SIGTERM has been caught! Exiting...");
    if(remove("run/daemon.pid") != 0)
    {
      syslog(LOG_ERR, "Failed to remove the pid file. Error number is %d", errno);
      exit(1);
    }
    exit(0);
  }
}

void handle_signals()
{
  if(signal(SIGTERM, sig_handler) == SIG_ERR)
  {
    syslog(LOG_ERR, "Error! Can't catch SIGTERM");
    exit(1);
  }
}

void daemonise()
{
  pid_t pid, sid;
  FILE *pid_fp;

  syslog(LOG_INFO, "Starting daemonisation.");

  //First fork
  pid = fork();
  if(pid < 0)
  {
    syslog(LOG_ERR, "Error occured in the first fork while daemonising. Error number is %d", errno);
    exit(1);
  }

  if(pid > 0)
  {
    syslog(LOG_INFO, "First fork successful (Parent)");
    exit(0);
  }
  syslog(LOG_INFO, "First fork successful (Child)");

  //Create a new session
  sid = setsid();
  if(sid < 0) 
  {
    syslog(LOG_ERR, "Error occured in making a new session while daemonising. Error number is %d", errno);
    exit(1);
  }
  syslog(LOG_INFO, "New session was created successfuly!");

  //Second fork
  pid = fork();
  if(pid < 0)
  {
    syslog(LOG_ERR, "Error occured in the second fork while daemonising. Error number is %d", errno);
    exit(1);
  }

  if(pid > 0)
  {
    syslog(LOG_INFO, "Second fork successful (Parent)");
    exit(0);
  }
  syslog(LOG_INFO, "Second fork successful (Child)");

  pid = getpid();

  //Change working directory to Home directory
  if(chdir(getenv("HOME")) == -1)
  {
    syslog(LOG_ERR, "Failed to change working directory while daemonising. Error number is %d", errno);
    exit(1);
  }

  //Grant all permisions for all files and directories created by the daemon
  umask(0);

  //Redirect std IO
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  if(open("/dev/null",O_RDONLY) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stdin while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(open("/dev/null",O_WRONLY) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stdout while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(open("/dev/null",O_RDWR) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stderr while daemonising. Error number is %d", errno);
    exit(1);
  }

  //Create a pid file
  mkdir("run/", 0777);
  pid_fp = fopen("run/daemon.pid", "w");
  if(pid_fp == NULL)
  {
    syslog(LOG_ERR, "Failed to create a pid file while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(fprintf(pid_fp, "%d\n", pid) < 0)
  {
    syslog(LOG_ERR, "Failed to write pid to pid file while daemonising. Error number is %d, trying to remove file", errno);
    fclose(pid_fp);
    if(remove("run/daemon.pid") != 0)
    {
      syslog(LOG_ERR, "Failed to remove pid file. Error number is %d", errno);
    }
    exit(1);
  }
  fclose(pid_fp);
}

int main(int argc, char * argv[])
{
  char *     path1 = NULL;
  char *     path2 = NULL;
  char       buffer[300];
  char       config[] = "./config.txt";
  int        pause;
  FILE *     configFile;
  FILE *     pidFile;
  int        pid;


  //parse cmd arguments
  if (argc == 2) {
    if (!strcmp(argv[1], "start")) {
      //open config file
      if ((configFile = fopen(config, "r")) == NULL) {
        exit(0);
      }

      chdir(getenv("HOME"));
      if ((pidFile = fopen("run/daemon.pid", "r")) == NULL) {
        //parse config file
        fscanf(configFile, "%s",buffer);
        path1 = (char *)malloc(strlen(buffer) + 1);
        strcpy(path1, buffer);

        fscanf(configFile, "%s", buffer);
        path2 = (char *)malloc(strlen(buffer) + 1);
        strcpy(path2, buffer);

        fscanf(configFile, "%d", &pause);
        fclose(configFile);

        daemonise();
        handle_signals();
      }
      else {
        fclose(pidFile);
        exit(0);             //file daemon.pid already exists
      }
    }
    else if (!strcmp(argv[1], "stop")) {
      chdir(getenv("HOME"));
      if ((pidFile = fopen("run/daemon.pid", "r")) == NULL) {
        exit(0);      //file daemon.pid doesn't exist
      }
      else {
        fscanf(pidFile, "%d", &pid);
        syslog(LOG_INFO, "Daemon was stopped");
        kill(pid, SIGTERM);
        fclose(pidFile);
        free(path1);
        free(path2);

        exit(0);      //daemon was stopped 
      }
    }
    else {
      exit(0);        //unknown command (argv[1] != start and argv[1] != stop)
    }
  }
  else {
    exit(0);          //argc != 2
  }

  //run moving of files
  while(1)
  {
    moveFiles(path1, path2, 0, 2);//lifetime < 2 minutes
    moveFiles(path2, path1, 1, 2);//lifetime > 2 minutes

    sleep(pause);
  }
  exit(0);
}
