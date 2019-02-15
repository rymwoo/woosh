#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include "../include/jobs.h"

using std::cout;

JobController* JobController::instance = NULL;

JobController::JobController() {
}
JobController::~JobController() {
}

JobController* JobController::getInstance() {
  if (instance==NULL)
    instance = new JobController();
  return instance;
}

void JobController::addJob(pid_t pid, char *name) {
  jobInfo info;
  info.jobPID = pid;
  info.jobName.assign(name);
  info.jobStatus = -1;
  jobList.push_back(info);
}

void JobController::printJobList() {
  if (jobList.empty())
    cout<<"No jobs to show.\n";
  else {
    for(int idx=0;idx<jobList.size();++idx) {
      cout<<"PID "<<jobList[idx].jobPID<<" "<<jobList[idx].jobName<<"\n";
    }
  }
}

void JobController::updateJobStatus() {
  int status;
  pid_t pid;
  do {
    pid = waitpid (WAIT_ANY, &status, WNOHANG);
    if (pid>0) {
      for(int i=0;i<jobList.size();++i) {
        if (jobList[i].jobPID == pid) {
          jobList[i].jobStatus = status;
          break;
        } // if (jobList[i].jobPID == pid)
      } // for
    }
  } while (pid>0);
}

bool JobController::anyBGJobsExited() {
  for (int i=0;i<jobList.size();++i){
    if (jobList[i].jobStatus>=0) {
      int status = jobList[i].jobStatus;
      if (WIFEXITED(status)||WIFSIGNALED(status)) {
        return true;
        break;
      }
    }
  }
  return false;
}

void JobController::cleanUpCompletedJobs() {
  int idx=0;
  do {
    if (jobList[idx].jobStatus>=0) {
      int status = jobList[idx].jobStatus;
      if (WIFEXITED(status)||WIFSIGNALED(status)) {
        cout<<"PID"<<jobList[idx].jobPID<<" "<<jobList[idx].jobName<<" ";
        if (WIFEXITED(status))
          cout<<"Child process terminated normally\n";
        else if (WIFSIGNALED(status)) {
          status = WTERMSIG(status);
          cout<<"Child process terminated by signal#["<<status<<"]\n";
        }
        jobList.erase(jobList.begin()+idx);
      } else
        idx++;
    } else //jobStatus exists
      idx++;
  } while (idx < jobList.size());
}

int JobController::jobListSz() {
  return jobList.size();
}