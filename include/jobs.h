#ifndef JOBS_H
#define JOBS_H

#include <vector>
#include <unistd.h>
#include <string>
//#include <utility>

using std::vector;
using std::string;

struct jobInfo {
  pid_t jobPID;
  string jobName;
  int jobStatus;
};
class JobController {
  public:
    static JobController* getInstance();
    ~JobController();
    
    void addJob(pid_t pid, char *name);
    void printJobList();
    void updateJobStatus();
    
    bool anyBGJobsExited();
    void cleanUpCompletedJobs();
    int jobListSz();
  private:
    JobController();
    static JobController* instance;
    vector<jobInfo> jobList;
};

#endif